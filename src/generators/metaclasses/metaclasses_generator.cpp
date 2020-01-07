#include "metaclasses_generator.h"
#include "ast_reflector.h"
#include "ast_utils.h"
#include "jinja2_reflection.h"
#include "cpp_interpreter.h"

#include <clang/ASTMatchers/ASTMatchers.h>

#include <boost/algorithm/string/predicate.hpp>

using namespace clang::ast_matchers;
using namespace clang;

namespace codegen
{
namespace
{
DeclarationMatcher metaclassesMatcher = cxxRecordDecl(isDerivedFrom("meta::detail::MetaClassImplBase")).bind("MetaclassImpl");


auto g_metaclassHdrTemplate =
R"(
{% extends "header_skeleton.j2tpl" %}

{% block namespaced_decls %}{{super()}}{% endblock %}

{% block namespace_content %}
{% for class in ns.classes %}

{% macro Decls(class, access) %}
{% for part in class.genericParts | selectattr('accessType', 'in', access) %}
    {{ part.content }};
{% endfor %}

{% for method in class.methods | rejectattr('isImplicit') | selectattr('accessType', 'in', access) %}
    {% if method.isTemplate %}template< {{method.tplParams | map(attribute='tplDeclName') | join(', ') }} >{% endif %}
    {{'explicit ' if method.isExplicitCtor}}{{'virtual ' if method.isVirtual}}{{'constexpr ' if method.isConstexpr}}{{'static ' if method.isStatic}}{{ method.returnType.printedName }} {{method.name}}({{method.params | map(attribute='fullDecl') | join(', ')}}){{'const ' if method.isConst}}{{'noexcept ' if method.isNoExcept}}{{'= 0' if method.isPure}}{{'= delete' if method.isDeleted}}{{'= default' if method.isDefault}}
    {% if method.isDefined and method.isClassScopeInlined %}{ {{method.body}} }{% else %};{% endif %}
{% endfor %}

{% for member in class.members | selectattr('accessType', 'in', access) %}
    {{'static ' if member.isStatic}}{{member.type.printedName}} {{ member.name }};
{% endfor %}
{% endmacro %}

{% if class.isTemplate %}template< {{class.tplParams | map(attribute='tplDeclName') | join(', ') }} >{% endif %}
class {{ class.name }}
{
public:
    {{ Decls(class, ['Public']) }}
protected:
    {{ Decls(class, ['Protected']) }}
private:
    {{ Decls(class, ['Private', 'Undefined']) }}
};

{% endfor %}
{% endblock %}
)";
}

MetaclassesGenerator::MetaclassesGenerator(const Options& opts)
    : BasicGenerator(opts)
{

}

void MetaclassesGenerator::SetupMatcher(clang::ast_matchers::MatchFinder& finder, clang::ast_matchers::MatchFinder::MatchCallback* defaultCallback)
{
    finder.addMatcher(metaclassesMatcher, defaultCallback);
}

void MetaclassesGenerator::HandleMatch(const clang::ast_matchers::MatchFinder::MatchResult& matchResult)
{
    if (m_astContext == nullptr)
        m_astContext = matchResult.Context;

    if (const clang::CXXRecordDecl* decl = matchResult.Nodes.getNodeAs<clang::CXXRecordDecl>("MetaclassImpl"))
    {
        if (IsFromInputFiles(decl->getBeginLoc(), matchResult.Context))
        {
            reflection::AstReflector reflector(matchResult.Context, m_options.consoleWriter);

            auto ci = reflector.ReflectClass(decl, &m_namespaces);

            dbg() << "### Implementation of metaclass found: " << ci->GetFullQualifiedName() << std::endl;
            ProcessMetaclassImplDecl(ci, matchResult.Context);
        }
    }

}

void MetaclassesGenerator::ProcessMetaclassDecl(reflection::ClassInfoPtr classInfo, const clang::ASTContext* astContext)
{
    const std::string metaclassNamePrefix = "MetaClass_";
    if (classInfo->name.find(metaclassNamePrefix) != 0)
        return;

    std::string metaclassName = classInfo->name.substr(metaclassNamePrefix.length());
    reflection::NamedDeclInfo tmpInfo = *classInfo;
    tmpInfo.name = metaclassName;
    std::string fullMetaclassName =  tmpInfo.GetFullQualifiedName();
    dbg() << "####### Metaclass found: " << fullMetaclassName << std::endl;

    MetaclassInfo& metaclassInfo = m_metaclasses[fullMetaclassName];

    reflection::ClassInfoPtr metaClassInfo;
    for (auto& innerDecl : classInfo->innerDecls)
    {
        auto ci = innerDecl.AsClassInfo();
        if (ci && ci->name == metaclassName && ci->hasDefinition)
        {
            metaClassInfo = ci;
            break;
        }
    }

    if (!metaClassInfo)
        return;

    metaclassInfo.metaclass = metaClassInfo;
//    metaClassInfo->decl->dump();

    for (reflection::MethodInfoPtr mi : metaClassInfo->methods)
    {
        if (mi->name == "GenerateDecl")
            metaclassInfo.declGenMethod = mi;
        else if (mi->name == "GenerateDef")
            metaclassInfo.defGenMethod = mi;

        dbg() << "@@@@@@@@ Metaclass declaration method: " << mi->name << std::endl;
//        if (mi->decl && !mi->isImplicit)
//            mi->decl->dump();
//        if (mi->name == "GenerateDecl")
//        {
//            dbg() << ">>>> Generate declaration method found. Method AST:";
//            mi->decl->dump();
//        }
    }
}

void MetaclassesGenerator::ProcessMetaclassImplDecl(reflection::ClassInfoPtr classInfo, const clang::ASTContext* astContext)
{
    reflection::AstReflector reflector(astContext, m_options.consoleWriter);

    const std::string metaclassNamePrefix = "MetaClassInstance_";
    if (classInfo->name.find(metaclassNamePrefix) != 0)
        return;

    std::string metaclassName = classInfo->name.substr(metaclassNamePrefix.length());

    // classInfo->decl->dump();

    reflection::ClassInfoPtr instInfo;
    std::vector<reflection::MethodInfoPtr> metaclassFunctions;

    for (auto& innerDecl : classInfo->innerDecls)
    {
        auto ci = innerDecl.AsClassInfo();
        if (ci && ci->name == metaclassName && ci->hasDefinition)
        {
            instInfo = ci;
            break;
        }
    }

    if (!instInfo)
        return;

    for (auto& memberDecl : classInfo->members)
    {
        if (memberDecl->name != "metaPtrList_")
            continue;

        auto varDecl = llvm::dyn_cast_or_null<clang::VarDecl>(memberDecl->decl);
        if (!varDecl)
            continue;

        if (!varDecl->hasInit())
            continue;

        auto e1 = llvm::dyn_cast_or_null<clang::ExprWithCleanups>(varDecl->getInit());
        auto e2 = llvm::dyn_cast_or_null<clang::CXXStdInitializerListExpr>(e1->getSubExpr());
        auto e3 = llvm::dyn_cast_or_null<clang::MaterializeTemporaryExpr>(e2->getSubExpr());
        auto e4 = llvm::dyn_cast_or_null<clang::InitListExpr>(e3->GetTemporaryExpr());
        if (e4 == nullptr)
            continue;

        for (auto i : e4->inits())
        {
            auto e5 = llvm::dyn_cast_or_null<clang::ImplicitCastExpr>(i);
            auto e6 = llvm::dyn_cast_or_null<clang::DeclRefExpr>(e5->getSubExpr());
            auto fnDecl = llvm::dyn_cast_or_null<clang::FunctionDecl>(e6->getDecl());

            auto info = reflector.ReflectMethod(fnDecl, &m_namespaces);
            dbg() << "####### Metaclass '" << info->GetFullQualifiedName() << "' instance found: " << metaclassName << std::endl;
            metaclassFunctions.push_back(info);
            fnDecl->dump();
        }
    }


    const DeclContext* nsContext = classInfo->decl->getEnclosingNamespaceContext();
    auto ns = m_implNamespaces.GetNamespace(nsContext);

    MetaclassImplInfo implInfo;
    implInfo.metaclasses = std::move(metaclassFunctions);
    implInfo.implDecl = instInfo;
    implInfo.ns = ns;

    m_implsInfo.push_back(implInfo);
}

bool MetaclassesGenerator::Validate()
{
    CppInterpreter interpreter(m_astContext, this);

    for (auto& instInfo : m_implsInfo)
    {
        reflection::ClassInfoPtr finalInstance = instInfo.implDecl;
        for (auto& fn : instInfo.metaclasses)
        {
            dbg() << ">>>>>> Processing instance '" << finalInstance->name << "' of metaclass '" << fn->name << "'" << std::endl;

            auto instClassInfo = std::make_shared<reflection::ClassInfo>();
            *static_cast<reflection::NamedDeclInfo*>(instClassInfo.get()) = *static_cast<reflection::NamedDeclInfo*>(finalInstance.get());
            *static_cast<reflection::LocationInfo*>(instClassInfo.get()) = *static_cast<reflection::LocationInfo*>(finalInstance.get());
            instClassInfo->name = finalInstance->name;

            interpreter.Execute(fn, instClassInfo, finalInstance);
            finalInstance = instClassInfo;
        }
        instInfo.impl = finalInstance;
        instInfo.ns->classes.push_back(finalInstance);
    }
    return true;
}

void MetaclassesGenerator::WriteHeaderContent(codegen::CppSourceStream& hdrOs)
{
    jinja2::ValuesMap params = {
        {"headerGuard", GetHeaderGuard(m_options.outputHeaderName)},
        {"rootNamespace", jinja2::Reflect(m_implNamespaces.GetRootNamespace())}
    };

    RenderStringTemplate(g_metaclassHdrTemplate, hdrOs, params);
}

} // codegen

codegen::GeneratorPtr CreateMetaclassesGen(const codegen::Options& opts)
{
    return std::make_unique<codegen::MetaclassesGenerator>(opts);
}


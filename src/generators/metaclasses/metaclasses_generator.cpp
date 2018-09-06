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
DeclarationMatcher metaclassesMatcher = anyOf(
            cxxRecordDecl(isDerivedFrom("meta::detail::MetaClassBase")).bind("MetaclassDecl"),
            cxxRecordDecl(isDerivedFrom("meta::detail::MetaClassImplBase")).bind("MetaclassImpl")
            );


auto g_metaclassHdrTemplate =
R"(
{% extends "header_skeleton.j2tpl" %}

{% block namespaced_decls %}{{super()}}{% endblock %}

{% block namespace_content %}
{% for class in ns.classes | sort(attribute="name") %}

{% macro MethodsDecl(class, access) %}
{% for method in class.methods | rejectattr('isImplicit') | selectattr('accessType', 'in', access) %}
    {{'explicit ' if method.isExplicitCtor}}{{'virtual ' if method.isVirtual}}{{'constexpr ' if method.isConstexpr}}{{'static ' if method.isStatic}}{{ method.returnType.printedName }} {{method.name}}({{method.params | map(attribute='fullDecl') | join(', ')}}){{'const ' if method.isConst}}{{'noexcept ' if method.isNoExcept}}{{'= 0' if method.isPure}}{{'= delete' if method.isDeleted}}{{'= default' if method.isDefault}};
{% endfor %}
{% endmacro %}

class {{ class.name }}
{
public:
    {{ MethodsDecl(class, ['Public']) }}
protected:
    {{ MethodsDecl(class, ['Protected']) }}
private:
    {{ MethodsDecl(class, ['Private', 'Undefined']) }}
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

    if (const clang::CXXRecordDecl* decl = matchResult.Nodes.getNodeAs<clang::CXXRecordDecl>("MetaclassDecl"))
    {
        if (IsFromInputFiles(decl->getLocStart(), matchResult.Context))
        {
            reflection::AstReflector reflector(matchResult.Context, m_options.consoleWriter);

            auto ci = reflector.ReflectClass(decl, &m_namespaces);

            dbg() << "### Declaration of metaclass found: " << ci->GetFullQualifiedName() << std::endl;
            ProcessMetaclassDecl(ci, matchResult.Context);
        }
    }
    if (const clang::CXXRecordDecl* decl = matchResult.Nodes.getNodeAs<clang::CXXRecordDecl>("MetaclassImpl"))
    {
        if (IsFromInputFiles(decl->getLocStart(), matchResult.Context))
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

    classInfo->decl->dump();

    reflection::ClassInfoPtr instInfo;
    MetaclassInfo metaclassInfo;
    for (auto& innerDecl : classInfo->innerDecls)
    {
        auto ci = innerDecl.AsClassInfo();
        if (ci && ci->name == metaclassName && ci->hasDefinition)
        {
            instInfo = ci;
        }
        auto ti = innerDecl.AsTypedefInfo();
        if (ti && ti->name == "Metaclass")
        {
            const reflection::RecordType* recordType = ti->aliasedType->getAsRecord();
            if (recordType == nullptr)
                continue;
            auto recordDecl = llvm::dyn_cast_or_null<CXXRecordDecl>(recordType->decl);
            if (!recordDecl)
                continue;

            reflection::ClassInfoPtr ci = reflector.ReflectClass(recordDecl, nullptr);
            std::string fullName = ci->GetFullQualifiedName();
            fullName.erase(fullName.end() - 5, fullName.end());
            dbg() << "######## Instance of metaclass " << fullName << std::endl;
            auto p = m_metaclasses.find(fullName);
            if (p == m_metaclasses.end())
                return;
            metaclassInfo = p->second;
        }
    }

    if (!instInfo)
        return;

    dbg() << "####### Metaclass '" << metaclassInfo.metaclass->GetFullQualifiedName() << "' instance found: " << metaclassName << std::endl;

    const DeclContext* nsContext = classInfo->decl->getEnclosingNamespaceContext();
    auto ns = m_implNamespaces.GetNamespace(nsContext);

    auto instClassInfo = std::make_shared<reflection::ClassInfo>();
    *static_cast<reflection::ClassInfo*>(instClassInfo.get()) = *static_cast<reflection::ClassInfo*>(instInfo.get());
    *static_cast<reflection::NamedDeclInfo*>(instClassInfo.get()) = *static_cast<reflection::NamedDeclInfo*>(classInfo.get());
    *static_cast<reflection::LocationInfo*>(instClassInfo.get()) = *static_cast<reflection::LocationInfo*>(classInfo.get());
    instClassInfo->name = instInfo->name;
    ns->classes.push_back(instClassInfo);

    MetaclassImplInfo implInfo;
    implInfo.metaclass = metaclassInfo.metaclass;
    implInfo.impl = instClassInfo;
    implInfo.implDecl = instInfo;
    implInfo.declGenMethod = metaclassInfo.declGenMethod;
    implInfo.defGenMethod = metaclassInfo.defGenMethod;

    m_implsInfo.push_back(implInfo);

    instInfo->decl->dump();

    for (reflection::MethodInfoPtr mi : instInfo->methods)
    {
        dbg() << "@@@@@@@@ Metaclass instance method: " << mi->fullPrototype << std::endl;
//        if (mi->decl && !mi->isImplicit)
//            mi->decl->dump();
//        if (mi->name == "GenerateDecl")
//        {
//            dbg() << ">>>> Generate declaration method found. Method AST:";
//            mi->decl->dump();
//        }
    }
}

bool MetaclassesGenerator::Validate()
{
    CppInterpreter interpreter(m_astContext, this);

    for (auto& instInfo : m_implsInfo)
    {
        dbg() << ">>>>>> Processing metaclass instance '" << instInfo.impl->name << "' of '" << instInfo.metaclass->name << "'" << std::endl;
        interpreter.Execute(instInfo.metaclass, instInfo.impl, instInfo.declGenMethod);
    }
    return true;
}

void MetaclassesGenerator::WriteHeaderContent(codegen::CppSourceStream& hdrOs)
{
    jinja2::Template tpl(&m_templateEnv);
    tpl.Load(g_metaclassHdrTemplate);

    jinja2::ValuesMap params = {
        {"inputFiles", jinja2::Reflect(m_options.inputFiles)},
        {"headerGuard", GetHeaderGuard(m_options.outputHeaderName)},
        {"rootNamespace", jinja2::Reflect(m_implNamespaces.GetRootNamespace())}
    };

    SetupCommonTemplateParams(params);

    tpl.Render(hdrOs, params);
}

} // codegen

codegen::GeneratorPtr CreateMetaclassesGen(const codegen::Options& opts)
{
    return std::make_unique<codegen::MetaclassesGenerator>(opts);
}


#include "metaclasses_generator.h"
#include "ast_reflector.h"
#include "ast_utils.h"
#include "jinja2_reflection.h"

#include <clang/ASTMatchers/ASTMatchers.h>

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

class {{ class.name }}
{
public:
    {% for method in class.methods | rejectattr('isImplicit') | selectattr('accessType', 'equalto', 'Public') %}
    {{ method.fullPrototype }};
    {% endfor %}
protected:
    {% for method in class.methods | rejectattr('isImplicit') | selectattr('accessType', 'equalto', 'Protected') %}
    {{ method.fullPrototype }};
    {% endfor %}
private:
    {% for method in class.methods | rejectattr('isImplicit') | selectattr('accessType', 'in', ['Private', 'Undefined']) %}
    {{ method.fullPrototype }};
    {% endfor %}
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
    if (const clang::CXXRecordDecl* decl = matchResult.Nodes.getNodeAs<clang::CXXRecordDecl>("MetaclassDecl"))
    {
        if (IsFromInputFiles(decl->getLocStart(), matchResult.Context))
        {
            reflection::AstReflector reflector(matchResult.Context);

            auto ci = reflector.ReflectClass(decl, &m_namespaces);

            std::cout << "### Declaration of metaclass found: " << ci->GetFullQualifiedName() << std::endl;
            ProcessMetaclassDecl(ci, matchResult.Context);
        }
    }
    if (const clang::CXXRecordDecl* decl = matchResult.Nodes.getNodeAs<clang::CXXRecordDecl>("MetaclassImpl"))
    {
        if (IsFromInputFiles(decl->getLocStart(), matchResult.Context))
        {
            reflection::AstReflector reflector(matchResult.Context);

            auto ci = reflector.ReflectClass(decl, &m_namespaces);

            std::cout << "### Implementation of metaclass found: " << ci->GetFullQualifiedName() << std::endl;
            ProcessMetaclassImplDecl(ci, matchResult.Context);
        }
    }

}

bool MetaclassesGenerator::Validate()
{
    return true;
}

void MetaclassesGenerator::ProcessMetaclassDecl(reflection::ClassInfoPtr classInfo, const clang::ASTContext* astContext)
{
    const std::string metaclassNamePrefix = "MetaClass_";
    if (classInfo->name.find(metaclassNamePrefix) != 0)
        return;

    std::string metaclassName = classInfo->name.substr(metaclassNamePrefix.length());
    std::cout << "####### Metaclass found: " << metaclassName << std::endl;

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

//    metaClassInfo->decl->dump();

    for (reflection::MethodInfoPtr mi : metaClassInfo->methods)
    {
        std::cout << "@@@@@@@@ Metaclass declaration method: " << mi->name << std::endl;
//        if (mi->decl && !mi->isImplicit)
//            mi->decl->dump();
//        if (mi->name == "GenerateDecl")
//        {
//            std::cout << ">>>> Generate declaration method found. Method AST:";
//            mi->decl->dump();
//        }
    }
}

void MetaclassesGenerator::ProcessMetaclassImplDecl(reflection::ClassInfoPtr classInfo, const clang::ASTContext* astContext)
{
    const std::string metaclassNamePrefix = "MetaClassInstance_";
    if (classInfo->name.find(metaclassNamePrefix) != 0)
        return;

    std::string metaclassName = classInfo->name.substr(metaclassNamePrefix.length());
    std::cout << "####### Metaclass instance found: " << metaclassName << std::endl;

    reflection::ClassInfoPtr instInfo;
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

    const DeclContext* nsContext = classInfo->decl->getEnclosingNamespaceContext();
    auto ns = m_implNamespaces.GetNamespace(nsContext);
    
    auto instClassInfo = std::make_shared<reflection::ClassInfo>();
    *static_cast<reflection::NamedDeclInfo*>(instClassInfo.get()) = *static_cast<reflection::NamedDeclInfo*>(classInfo.get());
    instClassInfo->name = instInfo->name;
    ns->classes.push_back(instClassInfo);
    
    instClassInfo->methods.insert(instClassInfo->methods.end(), instInfo->methods.begin(), instInfo->methods.end());
    
    instInfo->decl->dump();

    for (reflection::MethodInfoPtr mi : instInfo->methods)
    {
        std::cout << "@@@@@@@@ Metaclass instance method: " << mi->fullPrototype << std::endl;
//        if (mi->decl && !mi->isImplicit)
//            mi->decl->dump();
//        if (mi->name == "GenerateDecl")
//        {
//            std::cout << ">>>> Generate declaration method found. Method AST:";
//            mi->decl->dump();
//        }
    }
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


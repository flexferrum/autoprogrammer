#include "metaclasses_generator.h"
#include "ast_reflector.h"
#include "ast_utils.h"
#include "jinja2_reflection.h"

#include <clang/ASTMatchers/ASTMatchers.h>

using namespace clang::ast_matchers;

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

    metaClassInfo->decl->dump();

    for (reflection::MethodInfoPtr mi : metaClassInfo->methods)
    {
        std::cout << "@@@@@@@@ Metaclass declaration method: " << mi->name << std::endl;
        if (mi->decl && !mi->isImplicit)
            mi->decl->dump();
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
    };

    SetupCommonTemplateParams(params);

    tpl.Render(hdrOs, params);
}

} // codegen

codegen::GeneratorPtr CreateMetaclassesGen(const codegen::Options& opts)
{
    return std::make_unique<codegen::MetaclassesGenerator>(opts);
}


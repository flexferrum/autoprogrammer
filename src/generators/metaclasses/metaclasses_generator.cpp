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


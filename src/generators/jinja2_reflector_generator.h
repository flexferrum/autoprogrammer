#ifndef JINJA2_REFLECTOR_GENERATOR_H
#define JINJA2_REFLECTOR_GENERATOR_H

#include "basic_generator.h"
#include "decls_reflection.h"

namespace codegen
{
class Jinja2ReflectorGenerator : public BasicGenerator
{
public:
    struct MemberInfo
    {
        std::string reflectedName;
        std::string accessString;
    };

    struct ReflectedStructInfo
    {
        std::string name;
        std::string fullQualifiedName;
        std::vector<MemberInfo> members;
    };

    struct ReflectedEnumInfo
    {
        std::string name;
        std::string fullQualifiedName;
        std::string itemPrefix;
        std::vector<std::string> items;
    };

    Jinja2ReflectorGenerator(const Options& opts);

    // GeneratorBase interface
public:
    void SetupMatcher(clang::ast_matchers::MatchFinder &finder, clang::ast_matchers::MatchFinder::MatchCallback *defaultCallback) override;
    void HandleMatch(const clang::ast_matchers::MatchFinder::MatchResult &matchResult) override;
    bool Validate() override;

    // BasicGenerator interface
private:
    void PrepareStructInfo(reflection::ClassInfoPtr info, clang::ASTContext* context);
    void PrepareEnumInfo(reflection::EnumInfoPtr info, clang::ASTContext* context);
    void WriteHeaderPreamble(CppSourceStream &hdrOs) override;
    void WriteHeaderContent(CppSourceStream &hdrOs) override;
    void WriteHeaderPostamble(CppSourceStream &hdrOs) override;

private:
    reflection::NamespacesTree m_namespaces;
    std::vector<ReflectedStructInfo> m_reflectedStructs;
    std::vector<ReflectedEnumInfo> m_reflectedEnums;
    std::set<std::string> m_reflectedEnumsSet;
};
} // codegen

#endif // JINJA2_REFLECTOR_GENERATOR_H

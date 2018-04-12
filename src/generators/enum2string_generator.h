#ifndef ENUM2STRING_GENERATOR_H
#define ENUM2STRING_GENERATOR_H

#include "basic_generator.h"
#include "decls_reflection.h"

namespace codegen
{
class Enum2StringGenerator : public BasicGenerator
{
public:
    Enum2StringGenerator(const Options& opts);

    // GeneratorBase interface
    void SetupMatcher(clang::ast_matchers::MatchFinder &finder, clang::ast_matchers::MatchFinder::MatchCallback *defaultCallback) override;
    void HandleMatch(const clang::ast_matchers::MatchFinder::MatchResult &matchResult) override;

    // BasicGenerator interface
protected:
    void WriteHeaderPreamble(CppSourceStream &hdrOs) override;
    void WriteHeaderContent(CppSourceStream &hdrOs) override;
    void WriteHeaderPostamble(CppSourceStream &hdrOs) override;

private:

    void WriteEnumToStringConversion(CppSourceStream &hdrOs, const reflection::EnumInfoPtr& enumDescr);
    void WriteEnumFromStringConversion(CppSourceStream &hdrOs, const reflection::EnumInfoPtr& enumDescr);

private:
    reflection::NamespacesTree m_namespaces;
};
} // codegen

#endif // ENUM2STRING_GENERATOR_H

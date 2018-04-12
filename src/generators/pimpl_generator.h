#ifndef PIMPL_GENERATOR_H
#define PIMPL_GENERATOR_H

#include "basic_generator.h"
#include "decls_reflection.h"

namespace codegen
{
class PimplGenerator : public BasicGenerator
{
public:
    PimplGenerator(const Options& opts);

    // GeneratorBase interface
public:
    void SetupMatcher(clang::ast_matchers::MatchFinder &finder, clang::ast_matchers::MatchFinder::MatchCallback *defaultCallback) override;
    void HandleMatch(const clang::ast_matchers::MatchFinder::MatchResult &matchResult) override;
    bool Validate() override;

    // BasicGenerator interface
protected:
    void WriteHeaderPreamble(CppSourceStream &hdrOs) override;
    void WriteHeaderContent(CppSourceStream &hdrOs) override;
    void WriteHeaderPostamble(CppSourceStream &hdrOs) override;
    void WriteSourcePreamble(CppSourceStream &srcOs) override;
    void WriteSourceContent(CppSourceStream &srcOs) override;
    void WriteSourcePostamble(CppSourceStream &srcOs) override;

private:
    void WritePimplImplementation(CppSourceStream& os, reflection::ClassInfoPtr classInfo);
    void WriteCtorImplementation(CppSourceStream& os, reflection::ClassInfoPtr classInfo, reflection::MethodInfoPtr methodInfo);
    void WriteMethodImplementation(CppSourceStream& os, reflection::ClassInfoPtr classInfo, reflection::MethodInfoPtr methodInfo);

private:
    reflection::NamespacesTree m_namespaces;
};
} // codegen

#endif // PIMPL_GENERATOR_H

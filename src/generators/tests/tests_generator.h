#ifndef TESTS_GENERATOR_H
#define TESTS_GENERATOR_H

#include "generators/basic_generator.h"
#include "decls_reflection.h"

namespace codegen
{

namespace test_gen
{
class TestImplGenerator;
} // test_gen

class TestsGenerator : public BasicGenerator
{
public:
    TestsGenerator(const Options& opts);

    // GeneratorBase interface
    void SetupMatcher(clang::ast_matchers::MatchFinder& finder, clang::ast_matchers::MatchFinder::MatchCallback* defaultCallback) override;
    void HandleMatch(const clang::ast_matchers::MatchFinder::MatchResult& matchResult) override;

private:
    std::vector<std::unique_ptr<clang::ast_matchers::DeclarationMatcher>> m_matchers;
    reflection::NamespacesTree m_namespaces;

    enum class TestType
    {
        BasicPositive,
        BasicNegative,
        ComplexPositive,
        ComplexNegative
    };

    // BasicGenerator interface
protected:
    void WriteHeaderPreamble(CppSourceStream& hdrOs) override;
    void WriteHeaderContent(CppSourceStream& hdrOs) override;
    void WriteHeaderPostamble(CppSourceStream& hdrOs) override;
    void WriteSourcePreamble(CppSourceStream& srcOs) override;
    void WriteSourceContent(CppSourceStream& srcOs) override;
    void WriteSourcePostamble(CppSourceStream& srcOs) override;

private:
    void WriteTestHelperClass(CppSourceStream& os, reflection::ClassInfoPtr classInfo);
    void WriteTestsForClass(CppSourceStream& os, reflection::ClassInfoPtr classInfo);
    void WriteMethodTestCases(CppSourceStream& os, reflection::ClassInfoPtr classInfo, reflection::MethodInfoPtr methodInfo);
    void WriteCtorTestCases(CppSourceStream& os, reflection::ClassInfoPtr classInfo, reflection::MethodInfoPtr methodInfo);
    void WriteOperatorTestCases(CppSourceStream& os, reflection::ClassInfoPtr classInfo, reflection::MethodInfoPtr methodInfo);
    void WriteGenericMethodTestCases(CppSourceStream& os, reflection::ClassInfoPtr classInfo, reflection::MethodInfoPtr methodInfo);

    void WriteGenericConstructorTestImpl(CppSourceStream& os, reflection::ClassInfoPtr classInfo, reflection::MethodInfoPtr methodInfo, TestType testType);
    void WriteDefaultConstructorTestImpl(CppSourceStream& os, reflection::ClassInfoPtr classInfo, reflection::MethodInfoPtr methodInfo, TestType testType);
    void WriteCopyConstructorTestImpl(CppSourceStream& os, reflection::ClassInfoPtr classInfo, reflection::MethodInfoPtr methodInfo, TestType testType);
    void WriteMoveConstructorTestImpl(CppSourceStream& os, reflection::ClassInfoPtr classInfo, reflection::MethodInfoPtr methodInfo, TestType testType);
    void WriteConvertConstructorTestImpl(CppSourceStream& os, reflection::ClassInfoPtr classInfo, reflection::MethodInfoPtr methodInfo, TestType testType);

private:
    std::unique_ptr<test_gen::TestImplGenerator> m_implGenerator;
};
} // codegen

#endif // TESTS_GENERATOR_H

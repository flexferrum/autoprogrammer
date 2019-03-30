#ifndef GTEST_IMPL_GENERATOR_H
#define GTEST_IMPL_GENERATOR_H

#include "test_impl_generator.h"

namespace codegen
{
namespace test_gen
{
class GTestImplGenerator : public TestImplGenerator
{
public:
    GTestImplGenerator();

    // TestImplGenerator interface
public:
    void WriteSourcePreamble(CppSourceStream& srcOs) override;
    out::OutParams PrepareGlobalTestGenParams(reflection::ClassInfoPtr classInfo, const Options& options) override;
    void WriteClassTestPreamble(CppSourceStream& os, reflection::ClassInfoPtr classInfo) override;
    void WriteClassTestPostamble(CppSourceStream& os, reflection::ClassInfoPtr classInfo) override;
    void BeginTestCase(CppSourceStream& os, const std::string& name) override;
    void EndTestCase(CppSourceStream& os, const std::string& name) override;
    void WriteExpectExprIsTrue(CppSourceStream& os, std::string expr) override;
    void WriteExpectEqual(CppSourceStream& os, std::string expectedExpr, std::string actualExpr) override;
};
} // test_gen
}

#endif // GTEST_IMPL_GENERATOR_H

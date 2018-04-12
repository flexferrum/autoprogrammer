#include "gtest_impl_generator.h"

namespace codegen
{
namespace test_gen
{
GTestImplGenerator::GTestImplGenerator()
{

}

void test_gen::GTestImplGenerator::WriteSourcePreamble(CppSourceStream& srcOs)
{
    srcOs << out::new_line << "#include <gtest/gtest.h>\n";
}

out::OutParams GTestImplGenerator::PrepareGlobalTestGenParams(reflection::ClassInfoPtr classInfo, const Options& options)
{
    return {
        {"testDeclMacro", options.testGenOptions.testFixtureName.empty() ? "TEST" : "TEST_F"},
        {"fixtureName", options.testGenOptions.testFixtureName.empty() ? classInfo->name + "Test" : options.testGenOptions.testFixtureName}
    };
}

void GTestImplGenerator::WriteClassTestPreamble(CppSourceStream& os, reflection::ClassInfoPtr classInfo)
{

}

void GTestImplGenerator::WriteClassTestPostamble(CppSourceStream& os, reflection::ClassInfoPtr classInfo)
{

}

void GTestImplGenerator::BeginTestCase(CppSourceStream& os, const std::string& name)
{
    os << out::new_line << "$testDeclMacro$($fixtureName$, " << name << ")";
    os << out::new_line << "{";
    os << out::new_line << out::indent;
}

void GTestImplGenerator::EndTestCase(CppSourceStream& os, const std::string& name)
{
    os << out::unindent << out::new_line << "}";
}

void GTestImplGenerator::WriteExpectExprIsTrue(CppSourceStream& os, std::string expr)
{
    os << out::new_line << "EXPECT_TRUE(" << expr << ");";
}

void GTestImplGenerator::WriteExpectEqual(CppSourceStream& os, std::string expectedExpr, std::string actualExpr)
{
    os << out::new_line << "EXPECT_EQ(" << expectedExpr << ", " << actualExpr << ");";
}

} // test_gen
}

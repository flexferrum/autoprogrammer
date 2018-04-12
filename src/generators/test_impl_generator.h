#ifndef TEST_IMPL_GENERATOR_H
#define TEST_IMPL_GENERATOR_H

#include "cpp_source_stream.h"
#include "ast_reflector.h"
#include "options.h"

namespace codegen
{
namespace test_gen
{

enum class OperatorType
{
    Unknown,
    Assignment,
    CopyAssignment,
    MoveAssignment,
    Equal,
    LessThan,
    GreaterThan,
    NotLessThan,
    NotGreaterThan,
    Plus,
    PlusEqual,
    Minus,
    MinusEqual,
    Star,
    StarEqual,
    Dash,
    DashEqual,
    Hat,
    HatEqual,
    ShiftLeft,
    ShiftLeftEqual,
    ShiftRight,
    ShiftRightEqual,
    Ampersand,
    PreIncrement,
    PostIncrement,
    PreDecrement,
    PostDecrement,
    MemberAccess,
    MemberPointerAccessPtr,
    MemberPointerAccessRef,
    Index,
    FunctionCall
};

class TestImplGenerator
{
public:
    virtual ~TestImplGenerator() = default;

    virtual void WriteSourcePreamble(CppSourceStream& srcOs) = 0;
    virtual out::OutParams PrepareGlobalTestGenParams(reflection::ClassInfoPtr classInfo, const Options& options) = 0;
    virtual void WriteClassTestPreamble(CppSourceStream& os, reflection::ClassInfoPtr classInfo) = 0;
    virtual void WriteClassTestPostamble(CppSourceStream& os, reflection::ClassInfoPtr classInfo) = 0;
    virtual void BeginTestCase(CppSourceStream& os, const std::string& name) = 0;
    virtual void EndTestCase(CppSourceStream& os, const std::string& name) = 0;
    virtual void WriteExpectExprIsTrue(CppSourceStream& os, std::string expr) = 0;
    virtual void WriteExpectEqual(CppSourceStream& os, std::string expectedExpr, std::string actualExpr) = 0;
};
} // test_gen
}

#endif // TEST_IMPL_GENERATOR_H

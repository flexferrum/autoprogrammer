#include "tests_generator.h"
#include "options.h"
#include "ast_reflector.h"
#include "ast_utils.h"
#include "utils.h"

#include "gtest_impl_generator.h"

#include <clang/ASTMatchers/ASTMatchers.h>
#include <llvm/Support/Path.h>

#include <iostream>

using namespace clang::ast_matchers;

namespace codegen
{
TestsGenerator::TestsGenerator(const codegen::Options& opts)
    : BasicGenerator(opts)
{
    switch (opts.testGenOptions.testEngine)
    {
    case TestEngine::GoogleTests:
        m_implGenerator = std::make_unique<test_gen::GTestImplGenerator>();
        break;
    default:
        break;
    }
}

void TestsGenerator::SetupMatcher(MatchFinder& finder, MatchFinder::MatchCallback* defaultCallback)
{
    if (m_options.testGenOptions.classesToTest.empty())
    {
        m_matchers.push_back(std::make_unique<DeclarationMatcher>(cxxRecordDecl(isDefinition()).bind("regularClass")));
        finder.addMatcher(*m_matchers.back().get(), defaultCallback);
    }
}

void TestsGenerator::HandleMatch(const clang::ast_matchers::MatchFinder::MatchResult& matchResult)
{
    if (const clang::CXXRecordDecl* decl = matchResult.Nodes.getNodeAs<clang::CXXRecordDecl>("regularClass"))
    {
        if (!IsFromInputFiles(decl->getBeginLoc(), matchResult.Context) && !IsFromUpdatingFile(decl->getBeginLoc(), matchResult.Context))
            return;

        reflection::AstReflector reflector(matchResult.Context, m_options.consoleWriter);

        auto ci = reflector.ReflectClass(decl, &m_namespaces);

        dbg() << "### Declaration of class for test found: " << ci->GetFullQualifiedName() << std::endl;
        dbg() << "### Methods found: " << std::endl;
        for (auto& methodInfo : ci->methods)
        {
            dbg() << "###\tMethod: " << methodInfo->GetFullQualifiedName() << std::endl;
            dbg() << "###\tParams:" << std::endl;
            for (auto& paramInfo : methodInfo->params)
                dbg() << "###\t\t" << paramInfo.fullDecl << std::endl;
        }
        dbg() << "### --- found methods end" << std::endl;
    }
}

void TestsGenerator::WriteHeaderPreamble(codegen::CppSourceStream& hdrOs)
{
    hdrOs << out::header_guard(m_options.outputHeaderName) << "\n";

    // Include input files (directly, by path)
    for (auto fileName : m_options.inputFiles)
    {
        std::replace(fileName.begin(), fileName.end(), '\\', '/');
        hdrOs << "#include \"" << fileName << "\"\n";
    }

    WriteExtraHeaders(hdrOs);
    m_implGenerator->WriteSourcePreamble(hdrOs);
}

void TestsGenerator::WriteHeaderPostamble(codegen::CppSourceStream& hdrOs)
{
    hdrOs << out::scope_exit;
}

void TestsGenerator::WriteHeaderContent(codegen::CppSourceStream& hdrOs)
{
    WriteNamespaceContents(hdrOs, m_namespaces.GetRootNamespace(), [this](CppSourceStream &os, reflection::NamespaceInfoPtr ns) {
        for (auto& classInfo : ns->classes)
        {
            WriteTestHelperClass(os, classInfo);
        }
    });
}


void TestsGenerator::WriteTestHelperClass(CppSourceStream& os, reflection::ClassInfoPtr classInfo)
{
    out::ScopedParams scopedParams(os, {
            {"className", classInfo->GetScopedName()},
        });

    os << out::new_line(1) << "class $className$TestHelper";
    out::BracedStreamScope declBody("", ";\n");
    os << declBody;
    os << out::new_line(-1) << "public:";
    os << out::new_line(1) << "static bool IsDefault(const $className$& obj)";
    {
        out::BracedStreamScope funcBody("", "\n");
        os << funcBody;
        os << out::new_line(1) << "$className$ defValue{};";
        os << out::new_line(1) << "return AreEqual(defValue, obj);";
    }
    os << out::new_line(1) << "static bool AreEqual(const $className$& expected, const $className$& value)";
    {
        out::BracedStreamScope funcBody("", "\n");
        os << funcBody;
        os << out::new_line(1) << "bool result = true;";
        for (auto& member : classInfo->members)
        {
            auto left = "expected." + member->name;
            auto right = "value." + member->name;
            os << out::new_line(1) << "result &= " << left << " == " << right << ";";
            m_implGenerator->WriteExpectEqual(os, left, right);
        }
        os << out::new_line(1) << "return result;";
    }
}

void TestsGenerator::WriteSourcePreamble(codegen::CppSourceStream& srcOs)
{
    for (auto hdrPath : m_options.inputFiles)
    {
        std::string fileName = llvm::sys::path::filename(hdrPath).str();
        std::replace(fileName.begin(), fileName.end(), '\\', '/');
        srcOs << "#include \"" << fileName << "\"\n";
    }
    srcOs << "#include \"" << m_options.outputHeaderName << "\"\n";
    WriteExtraHeaders(srcOs);
    m_implGenerator->WriteSourcePreamble(srcOs);
}

void TestsGenerator::WriteSourceContent(codegen::CppSourceStream& srcOs)
{
    WriteNamespaceContents(srcOs, m_namespaces.GetRootNamespace(), [this](CppSourceStream &os, reflection::NamespaceInfoPtr ns) {
        for (auto& classInfo : ns->classes)
        {
            WriteTestsForClass(os, classInfo);
        }
    });
}

void TestsGenerator::WriteTestsForClass(CppSourceStream& os, reflection::ClassInfoPtr classInfo)
{
    out::ScopedParams globalParams(os, m_implGenerator->PrepareGlobalTestGenParams(classInfo, m_options));

    m_implGenerator->WriteClassTestPreamble(os, classInfo);

    for (auto& methodInfo : classInfo->methods)
    {
        WriteMethodTestCases(os, classInfo, methodInfo);
    }

    m_implGenerator->WriteClassTestPostamble(os, classInfo);
}

void TestsGenerator::WriteMethodTestCases(CppSourceStream& os, reflection::ClassInfoPtr classInfo, reflection::MethodInfoPtr methodInfo)
{
    if (methodInfo->isDeleted || methodInfo->isDtor)
        return;

    if (methodInfo->isCtor && !methodInfo->isDeleted)
    {
        WriteCtorTestCases(os, classInfo, methodInfo);
    }
    else if (methodInfo->isOperator && !methodInfo->isDeleted)
    {
        WriteOperatorTestCases(os, classInfo, methodInfo);
    }
    else if (methodInfo->accessType == reflection::AccessType::Public || m_options.testGenOptions.testPrivateMethods)
    {
        WriteGenericMethodTestCases(os, classInfo, methodInfo);
    }
}

void TestsGenerator::WriteCtorTestCases(CppSourceStream& os, reflection::ClassInfoPtr classInfo, reflection::MethodInfoPtr methodInfo)
{
    std::string testCaseName = classInfo->name + "_";

    using TestGeneratorPtr = void (TestsGenerator::*)(CppSourceStream& os, reflection::ClassInfoPtr classInfo, reflection::MethodInfoPtr methodInfo, TestType testType);
    TestGeneratorPtr generator = nullptr;

    switch (methodInfo->constructorType)
    {
    case reflection::ConstructorType::Generic:
        testCaseName += "Construct";
        generator = &TestsGenerator::WriteGenericConstructorTestImpl;
        break;
    case reflection::ConstructorType::Default:
        testCaseName += "DefaultConstruct";
        generator = &TestsGenerator::WriteDefaultConstructorTestImpl;
        break;
    case reflection::ConstructorType::Copy:
        testCaseName += "CopyConstruct";
        generator = &TestsGenerator::WriteCopyConstructorTestImpl;
        break;
    case reflection::ConstructorType::Move:
        testCaseName += "MoveConstruct";
        generator = &TestsGenerator::WriteMoveConstructorTestImpl;
        break;
    case reflection::ConstructorType::Convert:
        testCaseName += "ConvertConstruct";
        generator = &TestsGenerator::WriteConvertConstructorTestImpl;
        break;
    case reflection::ConstructorType::None:
    default:
        break;
    }

    if (generator == nullptr)
    {
        os << out::new_line << "// No generator for " << methodInfo->GetFullQualifiedName();
        return;
    }

    auto succeededCaseName = testCaseName + "_Succeeded";
    auto failedCaseName = testCaseName + "_Failed";

    if (m_options.testGenOptions.generateBasicPositive)
    {
        m_implGenerator->BeginTestCase(os, succeededCaseName);
        (this->*generator)(os, classInfo, methodInfo, TestType::BasicPositive);
        m_implGenerator->EndTestCase(os, succeededCaseName);
        os << "\n";
    }
    if (m_options.testGenOptions.generateBasicNegative)
    {
        m_implGenerator->BeginTestCase(os, failedCaseName);
        (this->*generator)(os, classInfo, methodInfo, TestType::BasicNegative);
        m_implGenerator->EndTestCase(os, failedCaseName);
        os << "\n";
    }
}

void TestsGenerator::WriteOperatorTestCases(CppSourceStream& os, reflection::ClassInfoPtr classInfo, reflection::MethodInfoPtr methodInfo)
{
    struct OperatorInfo
    {
        std::string testCaseName;
        test_gen::OperatorType operatorType;
    };

    static const std::map<std::string, OperatorInfo> operatorNames = {
        {"operator=", {"Assignment", test_gen::OperatorType::Assignment}},
        {"operator==", {"Equal", test_gen::OperatorType::Equal}},
        {"operator<", {"LessThan", test_gen::OperatorType::LessThan}},
        {"operator>", {"GreaterThan", test_gen::OperatorType::GreaterThan}},
        {"operator>=", {"NotLessThan", test_gen::OperatorType::NotLessThan}},
        {"operator<=", {"NotGreaterThan", test_gen::OperatorType::NotGreaterThan}},
        {"operator+", {"Plus", test_gen::OperatorType::Plus}},
        {"operator+=", {"PlusEqual", test_gen::OperatorType::PlusEqual}},
        {"operator-", {"Minus", test_gen::OperatorType::Minus}},
        {"operator-=", {"MinusEqual", test_gen::OperatorType::MinusEqual}},
        {"operator*", {"Star", test_gen::OperatorType::Star}},
        {"operator*=", {"StarEqual", test_gen::OperatorType::StarEqual}},
        {"operator/", {"Dash", test_gen::OperatorType::Dash}},
        {"operator/=", {"DashEqual", test_gen::OperatorType::DashEqual}},
        {"operator^", {"Hat", test_gen::OperatorType::Hat}},
        {"operator^=", {"HatEqual", test_gen::OperatorType::HatEqual}},
        {"operator<<", {"ShiftLeft", test_gen::OperatorType::ShiftLeft}},
        {"operator<<=", {"ShiftLeftEqual", test_gen::OperatorType::ShiftLeftEqual}},
        {"operator>>", {"ShiftRight", test_gen::OperatorType::ShiftRight}},
        {"operator>>=", {"ShiftRightEqual", test_gen::OperatorType::ShiftRightEqual}},
        {"operator&", {"Ampersand", test_gen::OperatorType::Ampersand}},
        {"operator++", {"PreIncrement", test_gen::OperatorType::PreIncrement}},
        {"operator--", {"PreDecrement", test_gen::OperatorType::PreDecrement}},
        {"operator->", {"MemberAccess", test_gen::OperatorType::MemberAccess}},
        {"operator->*", {"MemberPointerAccessPtr", test_gen::OperatorType::MemberPointerAccessPtr}},
        {"operator.*", {"MemberPointerAccessRef", test_gen::OperatorType::MemberPointerAccessRef}},
        {"operator[]", {"Index", test_gen::OperatorType::Index}},
        {"operator()", {"FunctionCall", test_gen::OperatorType::FunctionCall}},
    };

    test_gen::OperatorType operatorType = test_gen::OperatorType::Unknown;

    std::string testCaseName = classInfo->name + "_";
    switch (methodInfo->assignmentOperType)
    {
    case reflection::AssignmentOperType::Generic:
        testCaseName += "Assignment";
        operatorType = test_gen::OperatorType::Assignment;
        break;
    case reflection::AssignmentOperType::Copy:
        testCaseName += "CopyAssignment";
        operatorType = test_gen::OperatorType::CopyAssignment;
        break;
    case reflection::AssignmentOperType::Move:
        testCaseName += "MoveAssignment";
        operatorType = test_gen::OperatorType::MoveAssignment;
        break;
    case reflection::AssignmentOperType::None:
    default:
    {
        auto p = operatorNames.find(methodInfo->name);

        if (p != operatorNames.end())
        {
            testCaseName += p->second.testCaseName;
            operatorType = p->second.operatorType;
        }
        else
            testCaseName += "###UnknownOperator" + methodInfo->name;
        break;
    }
    }

    if (operatorType == test_gen::OperatorType::PreDecrement || operatorType == test_gen::OperatorType::PreIncrement)
    {
        if (!methodInfo->params.empty())
            operatorType = operatorType == test_gen::OperatorType::PreDecrement ? test_gen::OperatorType::PostDecrement : test_gen::OperatorType::PostIncrement;
    }

    auto succeededCaseName = testCaseName + "_Succeeded";
    auto failedCaseName = testCaseName + "_Failed";

    if (m_options.testGenOptions.generateBasicPositive)
    {
        m_implGenerator->BeginTestCase(os, succeededCaseName);
        m_implGenerator->EndTestCase(os, succeededCaseName);
        os << "\n";
    }
    if (m_options.testGenOptions.generateBasicNegative)
    {
        m_implGenerator->BeginTestCase(os, failedCaseName);
        m_implGenerator->EndTestCase(os, failedCaseName);
        os << "\n";
    }
}

void TestsGenerator::WriteGenericMethodTestCases(CppSourceStream& os, reflection::ClassInfoPtr classInfo, reflection::MethodInfoPtr methodInfo)
{
    std::string testCaseName = classInfo->name + "_";
    testCaseName += methodInfo->name;
    testCaseName += "_Succeeded";

    m_implGenerator->BeginTestCase(os, testCaseName);
    m_implGenerator->EndTestCase(os, testCaseName);
    os << "\n";
}

void TestsGenerator::WriteGenericConstructorTestImpl(CppSourceStream& os, reflection::ClassInfoPtr classInfo, reflection::MethodInfoPtr methodInfo, TestsGenerator::TestType testType)
{

}

void TestsGenerator::WriteDefaultConstructorTestImpl(CppSourceStream& os, reflection::ClassInfoPtr classInfo, reflection::MethodInfoPtr methodInfo, TestsGenerator::TestType testType)
{
    if (testType != TestType::BasicPositive)
    {
        os << out::new_line(1) << "// Nothing to do here";
        return;
    }
    os << out::new_line(1) << classInfo->name << " value{};";
    os << out::new_line(1) << "// Expectations for default object state go here";
}

void TestsGenerator::WriteCopyConstructorTestImpl(CppSourceStream& os, reflection::ClassInfoPtr classInfo, reflection::MethodInfoPtr methodInfo, TestsGenerator::TestType testType)
{
    if (testType != TestType::BasicPositive)
    {
        os << out::new_line(1) << "// Nothing to do here";
        return;
    }

    os << out::new_line(1) << classInfo->name << " initValue{};";
    os << out::new_line(1) << "// Special initialization goes here";
    os << out::new_line(2) << classInfo->name << " newValue(initValue);";
    m_implGenerator->WriteExpectExprIsTrue(os, classInfo->name + "TestHelper::AreEqual(initValue, newValue)");
//    os << out::new_line(1) << "// Test initValue and newValue are the same";
}

void TestsGenerator::WriteMoveConstructorTestImpl(CppSourceStream& os, reflection::ClassInfoPtr classInfo, reflection::MethodInfoPtr methodInfo, TestsGenerator::TestType testType)
{
    if (testType != TestType::BasicPositive)
    {
        os << out::new_line(1) << "// Nothing to do here";
        return;
    }

    os << out::new_line(1) << classInfo->name << " initValue{};";
    os << out::new_line(1) << "// Special initialization goes here";
    os << out::new_line(1) << classInfo->name << " initValueCopy(initValue);";
    os << out::new_line(2) << classInfo->name << " newValue(std::move(initValue));";
    m_implGenerator->WriteExpectExprIsTrue(os, classInfo->name + "TestHelper::AreEqual(initValueCopy, newValue)");
    m_implGenerator->WriteExpectExprIsTrue(os, classInfo->name + "TestHelper::IsDefault(initValue)");
}

void TestsGenerator::WriteConvertConstructorTestImpl(CppSourceStream& os, reflection::ClassInfoPtr classInfo, reflection::MethodInfoPtr methodInfo, TestsGenerator::TestType testType)
{

}

void TestsGenerator::WriteSourcePostamble(codegen::CppSourceStream& srcOs)
{
}

} // codegen

codegen::GeneratorPtr CreateTestGen(const codegen::Options& opts)
{
    return std::make_unique<codegen::TestsGenerator>(opts);
}

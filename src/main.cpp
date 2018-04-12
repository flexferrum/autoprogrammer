#include <clang/Tooling/Tooling.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/Diagnostic.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/JamCRC.h>
#include <llvm/Support/Path.h>

#include <iostream>
#include <fstream>
#include <algorithm>

#include "options.h"
#include "generator_base.h"

using namespace llvm;

extern codegen::GeneratorPtr CreateEnum2StringGen(const codegen::Options&);
extern codegen::GeneratorPtr CreatePimplGen(const codegen::Options&);
extern codegen::GeneratorPtr CreateTestGen(const codegen::Options&);

namespace
{
// Define code generation tool option category
cl::OptionCategory CodeGenCategory("Code generator options");

enum class TestGenOptions
{
    GenerateMocks,
    DontGenerateMocks,
    TestPrivateMethods,
    DontTestPrivateMethods,
    TestTemplates,
    DontTestTemplates,
    GenerateBasicPositive,
    DontGenerateBasicPositive,
    GenerateBasicNegative,
    DontGenerateBasicNegative,
    GenerateComplexPositive,
    DontGenerateComplexPositive,
    GenerateComplexNegative,
    DontGenerateComplexNegative,
};

// Define the generation mode
cl::opt<codegen::GeneratorId> GenerationMode(cl::desc("Choose generation mode:"),
  cl::values(
        clEnumValN(codegen::GeneratorId::Enum2StringGen, "gen-enum2string" , "Enum2string conversion generation"),
        clEnumValN(codegen::GeneratorId::PimplGen, "gen-pimpl" , "Pimpl wrapper classes generation"),
        clEnumValN(codegen::GeneratorId::TestsGen, "gen-tests" , "Test cases generation")
    ), cl::Required, cl::cat(CodeGenCategory));

// Define options for output file names
// cl::opt<std::string> OutputFilename("o", cl::desc("Specify output filename"), cl::value_desc("filename"), cl::cat(CodeGenCategory));
cl::opt<std::string> OutputHeaderName("ohdr", cl::desc("Specify output header filename"), cl::value_desc("filename"), cl::cat(CodeGenCategory));
cl::opt<std::string> OutputSourceName("osrc", cl::desc("Specify output source filename"), cl::value_desc("filename"), cl::cat(CodeGenCategory));
cl::opt<std::string> FileToUpdateName("update", cl::desc("Specify source filename for code update"), cl::value_desc("filename"), cl::cat(CodeGenCategory));
cl::opt<bool> ShowClangErrors("show-clang-diag", cl::desc("Show clang diagnostic during file processing"), cl::value_desc("flag"), cl::init(false), cl::cat(CodeGenCategory));
cl::list<std::string> ExtraHeaders("eh", cl::desc("Specify extra header files for include into generation result"), cl::value_desc("filename"), cl::ZeroOrMore, cl::cat(CodeGenCategory));
cl::list<TestGenOptions> TestGenModes("test-gen-mode", cl::desc("Tune up tests generation modes"),
  cl::values(
        clEnumValN(TestGenOptions::GenerateMocks, "mocks" , "Generate mocks (default)"),
        clEnumValN(TestGenOptions::DontGenerateMocks, "no-mocks" , "Don't generate mocks"),
        clEnumValN(TestGenOptions::TestPrivateMethods, "testprivate" , "Test protected and private methods (default)"),
        clEnumValN(TestGenOptions::DontTestPrivateMethods, "no-testprivate" , "Don't test protected and private methods"),
        clEnumValN(TestGenOptions::TestTemplates, "templates" , "Test template classes and methods"),
        clEnumValN(TestGenOptions::DontTestTemplates, "no-templates" , "Don't test template classes and methods (default)"),
        clEnumValN(TestGenOptions::GenerateBasicPositive, "basic-positive" , "Generate basic positive test cases (default)"),
        clEnumValN(TestGenOptions::DontGenerateBasicPositive, "no-basic-positive" , "Don't generate basic positive test cases"),
        clEnumValN(TestGenOptions::GenerateBasicNegative, "basic-negative" , "Generate basic negative test cases (default)"),
        clEnumValN(TestGenOptions::DontGenerateBasicNegative, "no-basic-negative" , "Don't generate basic negative test cases"),
        clEnumValN(TestGenOptions::GenerateComplexPositive, "complex-positive" , "Generate complex positive test cases (default)"),
        clEnumValN(TestGenOptions::DontGenerateComplexPositive, "no-complex-positive" , "Don't generate complex positive test cases"),
        clEnumValN(TestGenOptions::GenerateComplexNegative, "complex-negative" , "Generate complex negative test cases (default)"),
        clEnumValN(TestGenOptions::DontGenerateComplexNegative, "no-complex-negative" , "Don't generate complex negative test cases")
    ), cl::ZeroOrMore, cl::CommaSeparated, cl::cat(CodeGenCategory));
cl::list<std::string> ClassesToTest("test-class", cl::desc("Specify name of the class to test"), cl::value_desc("classname"), cl::ZeroOrMore, cl::CommaSeparated, cl::cat(CodeGenCategory));
cl::opt<std::string> TestFixtureName("test-fixture", cl::desc("Name of the test fixture"), cl::value_desc("classname"), cl::cat(CodeGenCategory));

cl::opt<codegen::Standard> LangStandart("std", cl::desc("Choose the standard conformance for the generation results:"),
  cl::values(
        clEnumValN(codegen::Standard::Auto, "auto" , "Automatic detection (default)"),
        clEnumValN(codegen::Standard::Cpp03, "c++03" , "C++ 2003 standard"),
        clEnumValN(codegen::Standard::Cpp11, "c++11" , "C++ 2011 standard"),
        clEnumValN(codegen::Standard::Cpp14, "c++14" , "C++ 2014 standard"),
        clEnumValN(codegen::Standard::Cpp17, "c++17" , "C++ 2017 standard")
    ), cl::Optional, cl::init(codegen::Standard::Auto), cl::cat(CodeGenCategory));

cl::opt<codegen::TestEngine> TestEngine("test-engine", cl::desc("Choose the destination test engine:"),
  cl::values(
        clEnumValN(codegen::TestEngine::GoogleTests, "gtest" , "Google test (default)"),
        clEnumValN(codegen::TestEngine::CppUnit, "cppunit" , "CppUnit"),
        clEnumValN(codegen::TestEngine::QtTest, "qttests" , "QtTests")
    ), cl::Optional, cl::init(codegen::TestEngine::GoogleTests), cl::cat(CodeGenCategory));

// Define common help message printer
cl::extrahelp CommonHelp(clang::tooling::CommonOptionsParser::HelpMessage);
// Define specific help message printer
cl::extrahelp MoreHelp("\nCode generation tool help text...");

std::vector<std::pair<codegen::GeneratorId, codegen::GeneratorFactory>> GenFactories = {
    {codegen::GeneratorId::Enum2StringGen, CreateEnum2StringGen},
    {codegen::GeneratorId::PimplGen, CreatePimplGen},
    {codegen::GeneratorId::TestsGen, CreateTestGen},
};
}

namespace codegen
{
class MatchHandler : public clang::ast_matchers::MatchFinder::MatchCallback
{
public:
    MatchHandler(const Options& options, GeneratorBase* generator)
        : m_options(options)
        , m_generator(generator)
    {
    }

    void onStartOfTranslationUnit()
    {
        m_generator->OnCompilationStarted();
    }

    void run(const clang::ast_matchers::MatchFinder::MatchResult& result)
    {
        m_generator->HandleMatch(result);
    }

    void onEndOfTranslationUnit()
    {
        if (!(m_hasErrors = !m_generator->Validate()))
            m_hasErrors = !m_generator->GenerateOutput();
    }

    bool HasErrors() const {return m_hasErrors;}

private:
    Options m_options;
    GeneratorBase* m_generator = nullptr;
    bool m_hasErrors = false;
};
} // codegen

int main(int argc, const char** argv)
{
    using namespace clang::tooling;
    using namespace clang::ast_matchers;

    clang::IgnoringDiagConsumer diagConsumer;

    // Parse command line options and setup ClangTool
    CommonOptionsParser optionsParser(argc, argv, CodeGenCategory);
    ClangTool tool(optionsParser.getCompilations(), optionsParser.getSourcePathList());

    codegen::Options options;
    options.generatorType = GenerationMode;
    options.inputFiles = optionsParser.getSourcePathList();
    options.outputHeaderName = OutputHeaderName;
    options.outputSourceName = OutputSourceName;
    options.targetStandard = LangStandart;
    options.extraHeaders = ExtraHeaders;
    options.testGenOptions.testEngine = TestEngine;
    for (auto& opt : TestGenModes)
    {
        switch (opt)
        {
        case TestGenOptions::GenerateMocks:
            options.testGenOptions.generateMocks = true;
            break;
        case TestGenOptions::DontGenerateMocks:
            options.testGenOptions.generateMocks = false;
            break;
        case TestGenOptions::TestPrivateMethods:
            options.testGenOptions.testPrivateMethods = true;
            break;
        case TestGenOptions::DontTestPrivateMethods:
            options.testGenOptions.testPrivateMethods = false;
            break;
        case TestGenOptions::TestTemplates:
            options.testGenOptions.testTemplates = true;
            break;
        case TestGenOptions::DontTestTemplates:
            options.testGenOptions.testTemplates = false;
            break;
        case TestGenOptions::GenerateBasicPositive:
            options.testGenOptions.generateBasicPositive = true;
            break;
        case TestGenOptions::DontGenerateBasicPositive:
            options.testGenOptions.generateBasicPositive = false;
            break;
        case TestGenOptions::GenerateBasicNegative:
            options.testGenOptions.generateBasicNegative = true;
            break;
        case TestGenOptions::DontGenerateBasicNegative:
            options.testGenOptions.generateBasicNegative = false;
            break;
        case TestGenOptions::GenerateComplexPositive:
            options.testGenOptions.generateComplexPositive = true;
            break;
        case TestGenOptions::DontGenerateComplexPositive:
            options.testGenOptions.generateComplexPositive = false;
            break;
        case TestGenOptions::GenerateComplexNegative:
            options.testGenOptions.generateComplexNegative = false;
            break;
        case TestGenOptions::DontGenerateComplexNegative:
            options.testGenOptions.generateComplexNegative = false;
            break;
        }
    }

    options.testGenOptions.classesToTest = ClassesToTest;
    options.testGenOptions.testFixtureName = TestFixtureName;

    if (!ShowClangErrors)
        tool.setDiagnosticConsumer(&diagConsumer);

    auto genFactory = std::find_if(begin(GenFactories), end(GenFactories), [type = options.generatorType](auto& p) {return p.first == type && p.second != nullptr;});

    if(genFactory == end(GenFactories))
    {
        std::cerr << "Generator is not defined for generation mode '" << GenerationMode.ValueStr.str() << "'" << std::endl;
        return -1;
    }

    auto generator = genFactory->second(options);

    if (!generator)
    {
        std::cerr << "Failed to create generator for generation mode '" << GenerationMode.ValueStr.str() << "'" << std::endl;
        return -1;
    }

    codegen::MatchHandler handler(options, generator.get());
    MatchFinder finder;
    generator->SetupMatcher(finder, &handler);


    // Run tool for the specified input files
    tool.run(newFrontendActionFactory(&finder).get());
    if (handler.HasErrors())
        return -1;

    return 0;
}

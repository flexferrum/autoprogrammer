#include <clang/Tooling/Tooling.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/Diagnostic.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/JamCRC.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FileUtilities.h>

#include <iostream>
#include <fstream>
#include <algorithm>

#include "options.h"
#include "generator_base.h"
#include "console_writer.h"

using namespace llvm;

extern codegen::GeneratorPtr CreateEnum2StringGen(const codegen::Options&);
extern codegen::GeneratorPtr CreatePimplGen(const codegen::Options&);
extern codegen::GeneratorPtr CreateJinja2ReflectGen(const codegen::Options&);
extern codegen::GeneratorPtr CreateTestGen(const codegen::Options&);
extern codegen::GeneratorPtr CreateMetaclassesGen(const codegen::Options&);

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
        clEnumValN(codegen::GeneratorId::Jinja2ReflectGen, "gen-jinja2reflect" , "Jinja2 reflection generation"),
        clEnumValN(codegen::GeneratorId::TestsGen, "gen-tests" , "Test cases generation"),
        clEnumValN(codegen::GeneratorId::MetaclassesGen, "gen-metaclasses" , "Test cases generation")
    ), cl::Required, cl::cat(CodeGenCategory));

// Define options for output file names
// cl::opt<std::string> OutputFilename("o", cl::desc("Specify output filename"), cl::value_desc("filename"), cl::cat(CodeGenCategory));
cl::opt<std::string> OutputHeaderName("ohdr", cl::desc("Specify output header filename"), cl::value_desc("filename"), cl::cat(CodeGenCategory));
cl::opt<std::string> OutputSourceName("osrc", cl::desc("Specify output source filename"), cl::value_desc("filename"), cl::cat(CodeGenCategory));
cl::opt<std::string> FileToUpdateName("update", cl::desc("Specify source filename for code update"), cl::value_desc("filename"), cl::cat(CodeGenCategory));
cl::opt<bool> ShowClangErrors("show-clang-diag", cl::desc("Show clang diagnostic during file processing"), cl::value_desc("flag"), cl::init(false), cl::cat(CodeGenCategory));
cl::opt<bool> RunInDebugMode("debug-mode", cl::desc("Show tool debug output"), cl::value_desc("flag"), cl::init(false), cl::cat(CodeGenCategory));
cl::list<std::string> ExtraHeaders("eh", cl::desc("Specify extra header files for include into generation result"), cl::value_desc("filename"), cl::ZeroOrMore, cl::cat(CodeGenCategory));
cl::list<std::string> InputFiles("input", cl::desc("Specify input files to process"), cl::value_desc("filename"), cl::ZeroOrMore, cl::cat(CodeGenCategory));
cl::opt<std::string> FormatStyle("format-style", cl::desc("Specify style name or configuration file name for the formatting style"), cl::init("LLVM"), cl::value_desc("stylename or filename"), cl::cat(CodeGenCategory));
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
    {codegen::GeneratorId::Jinja2ReflectGen, CreateJinja2ReflectGen},
    {codegen::GeneratorId::TestsGen, CreateTestGen},
    {codegen::GeneratorId::MetaclassesGen, CreateMetaclassesGen},
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
        if (!m_astContext)
        {
            m_astContext = result.Context;
            m_sourceManager = result.SourceManager;
        }
        m_generator->HandleMatch(result);
    }

    void onEndOfTranslationUnit()
    {
        if (!(m_hasErrors = !m_generator->Validate()))
            m_hasErrors = !m_generator->GenerateOutput(m_astContext, m_sourceManager);
    }

    bool HasErrors() const {return m_hasErrors;}

private:
    Options m_options;
    GeneratorBase* m_generator = nullptr;
    const clang::ASTContext* m_astContext = nullptr;
    clang::SourceManager* m_sourceManager = nullptr;
    bool m_hasErrors = false;
};
} // codegen

void ParseOptions(codegen::Options& options, clang::tooling::CommonOptionsParser& optionsParser)
{
    options.generatorType = GenerationMode;
    options.outputHeaderName = OutputHeaderName;
    options.outputSourceName = OutputSourceName;
    options.targetStandard = LangStandart;
    options.extraHeaders = ExtraHeaders;
    options.debugMode = RunInDebugMode;
    if (llvm::sys::fs::exists(static_cast<std::string>(FormatStyle)))
        options.formatStyleConfig = FormatStyle;
    else
        options.formatStyleName = FormatStyle;

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
}

void PrepareCommandLine(int argc, const char** argv, std::vector<const char*>& args, std::string& inputFileName)
{
    llvm::SmallVector<char, 128> tmpFileName;
    llvm::sys::fs::createTemporaryFile("codegen", "cpp", tmpFileName);
    inputFileName.assign(tmpFileName.begin(), tmpFileName.end());

    // Skip the tool-specific args
    int cur = 0;
    for (; cur < argc && strcmp(argv[cur], "--"); ++ cur)
        args.push_back(argv[cur]);

    args.push_back(inputFileName.c_str());
    if (cur == argc)
        return;

    args.push_back(argv[cur ++]);
    // TODO: Add additional options

    for (; cur < argc; ++ cur)
        args.push_back(argv[cur]);
}

std::string WriteInputFile(std::ostream& tempFile, const std::string& inputFile)
{
    llvm::SmallVector<char, 255> filePath(inputFile.begin(), inputFile.end());
    llvm::sys::fs::make_absolute(filePath);
    std::string result(filePath.begin(), filePath.end());
    llvm::sys::path::convert_to_slash(result, llvm::sys::path::Style::posix);
    tempFile << "#include \"" << result << "\"\n";

    return result;
}

int main(int argc, const char** argv)
{
    using namespace clang::tooling;
    using namespace clang::ast_matchers;

    clang::IgnoringDiagConsumer diagConsumer;

    // Parse command line options and setup ClangTool
    std::vector<const char*> clArgs;
    std::string inputFileName;
    PrepareCommandLine(argc, argv, clArgs, inputFileName);

    int argsCount = clArgs.size();
    CommonOptionsParser optionsParser(argsCount, &clArgs[0], CodeGenCategory);
    ClangTool tool(optionsParser.getCompilations(), optionsParser.getSourcePathList());

    codegen::Options options;
    ParseOptions(options, optionsParser);
    
    codegen::ConsoleWriter writer(options.debugMode, &std::cout);
    options.consoleWriter = &writer;
    
    if (!ShowClangErrors)
        tool.setDiagnosticConsumer(&diagConsumer);

    writer.DebugStream() << "Command line params:" << std::endl;
    for (auto& opt : clArgs)
    {
        writer.DebugStream() << "\t" << opt << std::endl;
    }

    auto genFactory = std::find_if(begin(GenFactories), end(GenFactories), [type = options.generatorType](auto& p) {return p.first == type && p.second != nullptr;});

    if(genFactory == end(GenFactories))
    {
        writer.ConsoleStream(codegen::MessageType::Fatal) << "Generator is not defined for generation mode '" << GenerationMode.ValueStr.str() << "'" << std::endl;
        return -1;
    }

    auto generator = genFactory->second(options);

    if (!generator)
    {
        writer.ConsoleStream(codegen::MessageType::Fatal) << "Failed to create generator for generation mode '" << GenerationMode.ValueStr.str() << "'" << std::endl;
        return -1;
    }

    llvm::FileRemover inputFileRemover(inputFileName);
    {
        std::ofstream input(inputFileName);
        for (auto& file : InputFiles)
            options.inputFiles.insert(WriteInputFile(input, file));

        if (!FileToUpdateName.getValue().empty())
            options.fileToUpdate = WriteInputFile(input, FileToUpdateName);
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

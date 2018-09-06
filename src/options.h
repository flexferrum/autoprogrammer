#ifndef OPTIONS_H
#define OPTIONS_H

#include <string>
#include <vector>
#include <set>

namespace codegen
{
class ConsoleWriter;

enum class Standard
{
    Auto,
    Cpp03,
    Cpp11,
    Cpp14,
    Cpp17
};

enum class GeneratorId
{
    Unknown = -1,
    Enum2StringGen,
    PimplGen,
    Jinja2ReflectGen,
    TestsGen,
    MetaclassesGen
};

enum class TestEngine
{
    GoogleTests,
    CppUnit,
    QtTest
};

struct TestGenOptions
{
    bool generateMocks = true;
    bool testPrivateMethods = true;
    bool testTemplates = false;
    bool generateBasicPositive = true;
    bool generateBasicNegative = true;
    bool generateComplexPositive = true;
    bool generateComplexNegative = true;
    TestEngine testEngine = TestEngine::GoogleTests;
    std::vector<std::string> classesToTest;
    std::string testFixtureName;
};

struct Options
{
    GeneratorId generatorType = GeneratorId::Unknown;
    std::string outputHeaderName;
    std::string outputSourceName;
    std::set<std::string> inputFiles;
    std::vector<std::string> extraHeaders;
    std::string fileToUpdate;
    std::string formatStyleName;
    std::string formatStyleConfig;
    bool debugMode = false;
    Standard targetStandard = Standard::Auto;
    TestGenOptions testGenOptions;
    ConsoleWriter* consoleWriter = nullptr;    
};

} // codegen

#endif // OPTIONS_H

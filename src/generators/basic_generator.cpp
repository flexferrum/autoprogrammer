#include "basic_generator.h"

#include <clang/AST/ASTContext.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/FileManager.h>
#include <clang/Rewrite/Frontend/Rewriters.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Format/Format.h>

#include <fstream>
#include <sstream>

namespace codegen
{
BasicGenerator::BasicGenerator(const Options &opts)
    : m_options(opts)
{

}

void BasicGenerator::OnCompilationStarted()
{

}

bool BasicGenerator::Validate()
{
    return true;
}

enum class FileState
{
    Good,
    Bad,
    NoFile
};

FileState OpenGeneratedFile(const std::string& fileName, std::ofstream& fileStream, std::ostream*& targetStream)
{
    if (fileName == "stdout")
    {
        targetStream = &std::cout;
        return FileState::Good;
    }

    if (fileName.empty())
        return FileState::NoFile;

    fileStream.open(fileName, std::ios_base::out | std::ios_base::trunc);
    if (!fileStream.good())
        return FileState::Bad;

    targetStream = &fileStream;
    return FileState::Good;
}

bool BasicGenerator::GenerateOutput(const clang::ASTContext* astContext, clang::SourceManager* sourceManager)
{
    if (!clang::format::getPredefinedStyle(m_options.formatStyleName, clang::format::FormatStyle::LK_Cpp, &m_formatStyle))
    {
        std::cout << "Can't load style with name: " << m_options.formatStyleName << std::endl;
    }
    bool result = true;
    if (!m_options.outputHeaderName.empty())
    {
        result = GenerateOutputFile(m_options.outputHeaderName, "fl-codegen-output-header.h", astContext, sourceManager, [this](auto& stream) {
            this->WriteHeaderPreamble(stream);
            this->WriteHeaderContent(stream);
            this->WriteHeaderPostamble(stream);

            return true;
        });
    }
    if (!result)
        return false;

    if (!m_options.outputSourceName.empty())
    {
        result = GenerateOutputFile(m_options.outputSourceName, "fl-codegen-output-source.cpp", astContext, sourceManager, [this](auto& stream) {
            this->WriteSourcePreamble(stream);
            this->WriteSourceContent(stream);
            this->WriteSourcePostamble(stream);

            return true;
        });
    }

    return result;
}

bool BasicGenerator::GenerateOutputFile(const std::string& fileName, std::string tmpFileId, const clang::ASTContext* astContext, clang::SourceManager* sourceManager, std::function<bool (CppSourceStream&)> generator)
{
    using namespace clang;

    std::ostringstream targetHeaderOs;
    {
        CppSourceStream stream(targetHeaderOs);

        if (!generator(stream))
            return false;
    }

    auto fileContent = targetHeaderOs.str();
    tooling::Range fileRange(0, static_cast<unsigned>(fileContent.size()));
    auto replaces = format::reformat(m_formatStyle, fileContent, {fileRange});
    auto formattingResult = tooling::applyAllReplacements(fileContent, replaces);

    if (!formattingResult)
        return false;
    std::string formattedContent = formattingResult.get();

    std::ofstream outHdrFile;
    std::ostream* targetOs;

    auto hdrFileState = OpenGeneratedFile(fileName, outHdrFile, targetOs);
    if (hdrFileState == FileState::Bad)
    {
        std::cerr << "Can't open output file for writing: " << fileName << std::endl;
        return false;
    }
    *targetOs << formattedContent;

    return true;
}

bool BasicGenerator::IsFromInputFiles(const clang::SourceLocation& loc, const clang::ASTContext* context) const
{
    auto& sm = context->getSourceManager();
    auto ploc = sm.getPresumedLoc(loc, false);
    std::string locFilename = ploc.getFilename();

    return m_options.inputFiles.count(locFilename) != 0;
}

bool BasicGenerator::IsFromUpdatingFile(const clang::SourceLocation& loc, const clang::ASTContext* context) const
{
    auto& sm = context->getSourceManager();
    auto ploc = sm.getPresumedLoc(loc, false);
    std::string locFilename = ploc.getFilename();

    return m_options.fileToUpdate == locFilename;
}

void BasicGenerator::WriteExtraHeaders(CppSourceStream& os)
{
    for (auto& h : m_options.extraHeaders)
    {
        os << "#include ";
        bool quoted = h[0] != '<';
        os << (quoted ? "\"" : "") << h << (quoted ? "\"" : "") << "\n";
    }
}

} // codegen

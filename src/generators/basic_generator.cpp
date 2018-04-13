#include "basic_generator.h"

#include <clang/AST/ASTContext.h>
#include <clang/Basic/SourceManager.h>

#include <fstream>

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

bool BasicGenerator::GenerateOutput()
{
    if (!m_options.outputHeaderName.empty())
    {
        std::ostream* targetHeaderOs = nullptr;
        std::ofstream outHdrFile;

        auto hdrFileState = OpenGeneratedFile(m_options.outputHeaderName, outHdrFile, targetHeaderOs);
        if (hdrFileState == FileState::Bad)
        {
            std::cerr << "Can't open output header file for writing: " << m_options.outputHeaderName << std::endl;
            return false;
        }

        if (hdrFileState == FileState::Good)
        {
            CppSourceStream stream(*targetHeaderOs);

            WriteHeaderPreamble(stream);
            WriteHeaderContent(stream);
            WriteHeaderPostamble(stream);
        }
    }

    if (!m_options.outputSourceName.empty())
    {
        std::ostream* targetSourceOs = nullptr;
        std::ofstream outSrcFile;

        auto hdrFileState = OpenGeneratedFile(m_options.outputSourceName, outSrcFile, targetSourceOs);
        if (hdrFileState == FileState::Bad)
        {
            std::cerr << "Can't open output source file for writing: " << m_options.outputSourceName << std::endl;
            return false;
        }

        if (hdrFileState == FileState::Good)
        {
            CppSourceStream stream(*targetSourceOs);

            WriteSourcePreamble(stream);
            WriteSourceContent(stream);
            WriteSourcePostamble(stream);
        }
    }

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

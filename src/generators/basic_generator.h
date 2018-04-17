#ifndef BASIC_GENERATOR_H
#define BASIC_GENERATOR_H

#include "generator_base.h"
#include "cpp_source_stream.h"

#include <clang/Format/Format.h>

namespace codegen
{
class BasicGenerator : public GeneratorBase
{
public:
    BasicGenerator(const Options& opts);

    // GeneratorBase interface
    void OnCompilationStarted() override;
    bool Validate() override;
    bool GenerateOutput(const clang::ASTContext* astContext, clang::SourceManager* sourceManager) override;

protected:
    const Options& m_options;

    bool IsFromInputFiles(const clang::SourceLocation& loc, const clang::ASTContext* context) const;
    bool IsFromUpdatingFile(const clang::SourceLocation& loc, const clang::ASTContext* context) const;

    virtual void WriteHeaderPreamble(CppSourceStream& hdrOs) {}
    virtual void WriteHeaderContent(CppSourceStream& hdrOs) {}
    virtual void WriteHeaderPostamble(CppSourceStream& hdrOs) {}

    virtual void WriteSourcePreamble(CppSourceStream& srcOs) {}
    virtual void WriteSourceContent(CppSourceStream& srcOs) {}
    virtual void WriteSourcePostamble(CppSourceStream& srcOs) {}

    void WriteExtraHeaders(CppSourceStream& os);

private:
    bool GenerateOutputFile(const std::string& fileName, std::string tmpFileId, const clang::ASTContext* astContext, clang::SourceManager* sourceManager, std::function<bool (CppSourceStream&)> generator);
    clang::format::FormatStyle m_formatStyle;
};

} // codegen

#endif // BASIC_GENERATOR_H

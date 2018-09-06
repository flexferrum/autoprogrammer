#ifndef BASIC_GENERATOR_H
#define BASIC_GENERATOR_H

#include "generator_base.h"
#include "cpp_source_stream.h"
#include "console_writer.h"

#include "diagnostic_reporter.h"
#include <jinja2cpp/filesystem_handler.h>
#include <jinja2cpp/template_env.h>

#include <clang/Format/Format.h>

namespace codegen
{
class BasicGenerator : public GeneratorBase, public IDiagnosticReporter
{
public:
    BasicGenerator(const Options& opts);

    // GeneratorBase interface
    void OnCompilationStarted() override;
    bool Validate() override;
    bool GenerateOutput(const clang::ASTContext* astContext, clang::SourceManager* sourceManager) override;

protected:
    const Options& m_options;
    jinja2::MemoryFileSystem m_inMemoryTemplates;
    jinja2::TemplateEnv m_templateEnv;

    bool IsFromInputFiles(const clang::SourceLocation& loc, const clang::ASTContext* context) const;
    bool IsFromUpdatingFile(const clang::SourceLocation& loc, const clang::ASTContext* context) const;

    virtual void WriteHeaderPreamble(CppSourceStream& hdrOs) {}
    virtual void WriteHeaderContent(CppSourceStream& hdrOs) {}
    virtual void WriteHeaderPostamble(CppSourceStream& hdrOs) {}

    virtual void WriteSourcePreamble(CppSourceStream& srcOs) {}
    virtual void WriteSourceContent(CppSourceStream& srcOs) {}
    virtual void WriteSourcePostamble(CppSourceStream& srcOs) {}

    void WriteExtraHeaders(CppSourceStream& os);

    std::string GetHeaderGuard(const std::string& filePath);
    void SetupCommonTemplateParams(jinja2::ValuesMap& params);

    void Report(MessageType type, const std::string fileName, unsigned line, unsigned col, std::string message) override;
    void Report(MessageType type, const reflection::SourceLocation& loc, std::string message) override;
    void Report(MessageType type, const clang::SourceLocation& loc, const clang::ASTContext* astContext, std::string message) override;
    std::ostream& GetDebugStream() override {return dbg();}
    
    std::ostream& dbg() {return m_options.consoleWriter->DebugStream();}
    
private:
    bool GenerateOutputFile(const std::string& fileName, std::string tmpFileId, const clang::ASTContext* astContext, clang::SourceManager* sourceManager, std::function<bool (CppSourceStream&)> generator);
    clang::format::FormatStyle m_formatStyle;
};

} // codegen

#endif // BASIC_GENERATOR_H

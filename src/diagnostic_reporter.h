#ifndef DIAGNOSTIC_REPORTER_H
#define DIAGNOSTIC_REPORTER_H

#include "console_writer.h"

#include <string>

namespace reflection
{
struct SourceLocation;
}

namespace clang
{
class ASTContext;
class SourceLocation;
} // namespace clang

namespace codegen
{
class IDiagnosticReporter
{
public:
    virtual void Report(MessageType type, const std::string fileName, unsigned line, unsigned col, std::string message) = 0;
    virtual void Report(MessageType type, const reflection::SourceLocation& loc, std::string message) = 0;
    virtual void Report(MessageType type, const clang::SourceLocation& loc, const clang::ASTContext* astContext, std::string message) = 0;
    virtual std::ostream& GetDebugStream() = 0;
    virtual ConsoleWriter* GetConsoleWriter() = 0;
};
} // codegen


#endif // DIAGNOSTIC_REPORTER_H

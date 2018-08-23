#ifndef DIAGNOSTIC_REPORTER_H
#define DIAGNOSTIC_REPORTER_H

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
enum class Diag
{
    Notice,
    Warning,
    Error
};

class IDiagnosticReporter
{
public:
    virtual void Report(Diag type, const std::string fileName, unsigned line, unsigned col, std::string message) = 0;
    virtual void Report(Diag type, const reflection::SourceLocation& loc, std::string message) = 0;
    virtual void Report(Diag type, const clang::SourceLocation& loc, const clang::ASTContext* astContext, std::string message) = 0;
};
} // codegen


#endif // DIAGNOSTIC_REPORTER_H

#ifndef CPP_INTERPRETER_IMPL_H
#define CPP_INTERPRETER_IMPL_H

#include "decls_reflection.h"
#include "diagnostic_reporter.h"
#include "value.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/StmtCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/APValue.h>

#include <nonstd/expected.hpp>

#include <stack>
#include <unordered_map>

namespace codegen
{
namespace interpreter
{
struct ScopeInfo
{
};

using ScopeStack = std::stack<ScopeInfo>;

template<bool IsFullExpr>
class RAIIScope
{
public:
    explicit RAIIScope(ScopeStack& stack)
        : m_stack(stack)
    {
        m_stack.push(ScopeInfo());
    }
    ~RAIIScope()
    {
        auto& scope = m_stack.top();
        Cleanup(scope);
        m_stack.pop();
    }
private:
    void Cleanup(ScopeInfo& scope)
    {
    }
private:
    ScopeStack& m_stack;
};

class InterpreterImpl
{
public:
    InterpreterImpl(const clang::ASTContext* astContext, reflection::ClassInfoPtr metaclass, reflection::ClassInfoPtr inst)
        : m_astContext(astContext)
        , m_metaclass(metaclass)
        , m_instance(inst)
    {
    }

    void ExecuteMethod(const clang::CXXMethodDecl* method);

private:
    enum ExecStatementResult
    {
      /// Evaluation failed.
      ESR_Failed,
      /// Hit a 'return' statement.
      ESR_Returned,
      /// Evaluation succeeded.
      ESR_Succeeded,
      /// Hit a 'continue' statement.
      ESR_Continue,
      /// Hit a 'break' statement.
      ESR_Break,
      /// Still scanning for 'case' or 'default' statement.
      ESR_CaseNotFound
    };


    // Executors
    ExecStatementResult ExecuteStatement(const clang::Stmt* stmt, const clang::SwitchCase* curSwithCase);
    bool ExecuteExpression(const clang::Expr* expr, Value& result);

    nonstd::expected<Value, std::string> GetDeclReference(const clang::NamedDecl* decl);
    bool DetectSpecialDecl(const clang::NamedDecl* decl, Value& val);

    bool Report(Diag type, const clang::SourceLocation& loc, std::string message);


private:
    using BlockScopeRAII = RAIIScope<false>;
    using ExprScopeRAII = RAIIScope<true>;

    const clang::ASTContext* m_astContext;
    reflection::ClassInfoPtr m_metaclass;
    reflection::ClassInfoPtr m_instance;
    std::stack<ScopeInfo> m_scopes;
    IDiagnosticReporter* m_diagReporter;
    std::unordered_map<const clang::NamedDecl*, Value::InternalRef> m_visibleDecls;
    std::unordered_map<std::string, Value> m_globalVars;

    friend class ExpressionEvaluator;
};


} // interpreter
} // codegen4

#endif // CPP_INTERPRETER_IMPL_H

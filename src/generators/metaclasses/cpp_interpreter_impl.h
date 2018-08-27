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
struct ScopeStack
{
    struct DeclInfo
    {
        const clang::NamedDecl* decl;
        Value val;
        bool isLifetimeExtended = false;
    };

    std::vector<DeclInfo> stack;
    std::unordered_map<const clang::NamedDecl*, Value::InternalRef>* m_visibleDecls;
};

template<bool IsFullExpr>
class RAIIScope
{
public:
    explicit RAIIScope(ScopeStack& stack)
        : m_stack(stack)
        , m_savedEnd(stack.stack.size())
    {
    }
    ~RAIIScope()
    {
        // Cleanup();
    }
private:
    void Cleanup()
    {
        auto newEnd = m_savedEnd;
        auto& stack = m_stack.stack;
        for (size_t i = m_savedEnd, n = stack.size(); i != n; ++ i)
        {
            if (IsFullExpr && stack[i].isLifetimeExtended)
            {
                // Full-expression cleanup of a lifetime-extended temporary: nothing
                // to do, just move this cleanup to the right place in the stack.
                std::swap(stack[i], stack[newEnd ++]);
                ++newEnd;
            }
            else
            {
                // End the lifetime of the object.
                m_stack.m_visibleDecls->erase(stack[i].decl);
            }
        }
        stack.erase(stack.begin() + newEnd, stack.end());
    }
private:
    ScopeStack& m_stack;
    size_t m_savedEnd;
};

class InterpreterImpl
{
public:
    InterpreterImpl(const clang::ASTContext* astContext, reflection::ClassInfoPtr metaclass, reflection::ClassInfoPtr inst)
        : m_astContext(astContext)
        , m_metaclass(metaclass)
        , m_instance(inst)
    {
        m_scopes.m_visibleDecls = &m_visibleDecls;
    }

    void ExecuteMethod(const clang::CXXMethodDecl* method);
    bool Report(Diag type, const clang::SourceLocation& loc, std::string message);
    clang::PrintingPolicy GetDefaultPrintingPolicy();


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
    ExecStatementResult EvaluateLoopBody(const clang::Stmt* body, const clang::SwitchCase *curSwitchCase = nullptr);

    bool ExecuteExpression(const clang::Expr* expr, Value& result);
    bool ExecuteAsBooleanCondition(const clang::Expr* expr, bool& result);
    bool ExecuteVarDecl(const clang::VarDecl *decl);
    bool ExecuteDecl(const clang::Decl *D);

    ScopeStack::DeclInfo CreateLocalVar(const clang::VarDecl *decl);

    nonstd::expected<Value, std::string> GetDeclReference(const clang::NamedDecl* decl);
    bool DetectSpecialDecl(const clang::NamedDecl* decl, Value& val);


private:
    using BlockScopeRAII = RAIIScope<false>;
    using ExprScopeRAII = RAIIScope<true>;

    const clang::ASTContext* m_astContext;
    reflection::ClassInfoPtr m_metaclass;
    reflection::ClassInfoPtr m_instance;
    ScopeStack m_scopes;
    IDiagnosticReporter* m_diagReporter;
    std::unordered_map<const clang::NamedDecl*, Value::InternalRef> m_visibleDecls;
    std::unordered_map<std::string, Value> m_globalVars;

    friend class ExpressionEvaluator;
};

} // interpreter
} // codegen4

#endif // CPP_INTERPRETER_IMPL_H

#ifndef EXPRESSSION_EVALUATOR_H
#define EXPRESSSION_EVALUATOR_H

#include "cpp_interpreter_impl.h"
#include "diagnostic_reporter.h"
#include <clang/AST/Expr.h>
#include <clang/AST/StmtVisitor.h>

#include <iostream>

namespace codegen
{
namespace interpreter
{

class ExpressionEvaluator : public clang::ConstStmtVisitor<ExpressionEvaluator>
{
public:
    ExpressionEvaluator(InterpreterImpl* interpreter, Value& result, bool& isOk);

    void VisitCXXMemberCallExpr(const clang::CXXMemberCallExpr* expr);
    void VisitCXXConstructExpr(const clang::CXXConstructExpr* expr);
    void VisitCXXOperatorCallExpr(const clang::CXXOperatorCallExpr* expr);
    void VisitDeclRefExpr(const clang::DeclRefExpr* expr);
    void VisitImplicitCastExpr(const clang::ImplicitCastExpr* expr);
    void VisitStringLiteral(const clang::StringLiteral* expr);
    void VisitExprWithCleanups(const clang::ExprWithCleanups* expr);
    void VisitMaterializeTemporaryExpr(const clang::MaterializeTemporaryExpr* expr);
    void VisitBinaryOperator(const clang::BinaryOperator* expr);
    void VisitUnaryOperator(const clang::UnaryOperator* expr);
    void VisitParenExpr(const clang::ParenExpr* expr);

private:
    auto& dbg() {return m_interpreter->dbg();}
    
    struct VisitorScope
    {
        ExpressionEvaluator* evaluator;
        bool isFirst = false;
        const char* scopeName;
        bool isSubmitted = false;

        VisitorScope(ExpressionEvaluator* eval, const char* name)
            : evaluator(eval)
            , scopeName(name)
        {
            eval->dbg() << "[ExpressionEvaluator] Enter " << name << std::endl;
            isFirst = eval->m_isFirst;
            eval->m_isFirst = false;

            if (isFirst)
                eval->m_currentValue = &eval->m_resultValue;
        }
        ~VisitorScope()
        {
            evaluator->dbg() << "[ExpressionEvaluator] Exit " << scopeName << ". IsSucceeded: " << evaluator->m_evalResult << std::endl;
        }
        void Submit(Value val)
        {
            if (evaluator->m_currentValue && evaluator->m_evalResult)
            {
                evaluator->dbg() << "[ExpressionEvaluator] Value successfully submitted. ValType: " << val.GetValue().which() << std::endl;
                *evaluator->m_currentValue = std::move(val);
            }
        }

    };

    bool EvalSubexpr(const clang::Expr* expr, Value& val);
    bool CalculateCallArgs(const clang::Expr* const* args, unsigned numArgs, std::vector<Value>& argValues);
    void ReportError(const clang::SourceLocation& loc, const std::string& errMsg);

private:
    bool m_isFirst = true;
    InterpreterImpl* m_interpreter;
    Value& m_resultValue;
    Value* m_currentValue = nullptr;
    bool& m_evalResult;
};

} // interpreter
} // codegen

#endif // EXPRESSSION_EVALUATOR_H

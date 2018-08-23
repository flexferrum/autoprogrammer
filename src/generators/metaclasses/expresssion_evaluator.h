#ifndef EXPRESSSION_EVALUATOR_H
#define EXPRESSSION_EVALUATOR_H

#include "cpp_interpreter_impl.h"
#include "diagnostic_reporter.h"
#include <clang/AST/Expr.h>
#include <clang/AST/StmtVisitor.h>

namespace codegen
{
namespace interpreter
{

class ExpressionEvaluator : public clang::ConstStmtVisitor<ExpressionEvaluator>
{
public:
    ExpressionEvaluator(InterpreterImpl* interpreter, Value& result, bool& isOk);

    void VisitCXXMemberCallExpr(const clang::CXXMemberCallExpr* expr);

private:
    bool EnterVisitor(Value& val);
    void ExitVisitor(bool isFirst, Value& val);

    bool CalculateCallArgs(const clang::CallExpr* expr, std::vector<Value>& args);

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

#include "expresssion_evaluator.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>

#include <iostream>

using namespace clang;

namespace codegen
{
namespace interpreter
{

ExpressionEvaluator::ExpressionEvaluator(InterpreterImpl* interpreter, Value& result, bool& isOk)
    : m_interpreter(interpreter)
    , m_resultValue(result)
    , m_evalResult(isOk)
{
}

bool ExpressionEvaluator::EnterVisitor(Value& val)
{
    bool result = m_isFirst;
    if (m_isFirst)
        m_isFirst = false;

    m_currentValue = &val;

    return result;
}

void ExpressionEvaluator::ExitVisitor(bool isFirst, Value& val)
{
    if (isFirst)
    {
        m_resultValue = std::move(val);
    }
}

void ExpressionEvaluator::VisitCXXMemberCallExpr(const CXXMemberCallExpr* expr)
{
    std::cout << "[ExpressionEvaluator] Enter VisitCXXMemberCallExpr" << std::endl;
    Value result;
    bool isFirst = EnterVisitor(result);
    const clang::CXXRecordDecl* rec = expr->getRecordDecl();
    const clang::CXXMethodDecl* method = expr->getMethodDecl();

    std::string recName = rec->getQualifiedNameAsString();
    std::cout << "[ExpressionEvaluator] Call method '" << method->getNameAsString() << "' from '" << recName << "'" << std::endl;

    std::vector<Value> args;
    if (CalculateCallArgs(expr, args))
    {
        ;
    }

    ExitVisitor(isFirst, result);
    std::cout << "[ExpressionEvaluator] Exit VisitCXXMemberCallExpr" << std::endl;
}

bool ExpressionEvaluator::CalculateCallArgs(const clang::CallExpr* expr, std::vector<Value>& args)
{
    int argNum = 0;
    for (auto& arg: expr->arguments())
    {
        std::cout << "[ExpressionEvaluator] Enter evaluation of arg #" << argNum << "'" << std::endl;
        Value val;
        bool isFirst = EnterVisitor(val);
        Visit(arg);
        if (!m_evalResult)
            return false;
        ExitVisitor(isFirst, val);
        args.push_back(std::move(val));
        std::cout << "[ExpressionEvaluator] Exit evaluation of arg #" << argNum << "'" << std::endl;
        argNum ++;
    }
}


} // interpreter
} // codegen

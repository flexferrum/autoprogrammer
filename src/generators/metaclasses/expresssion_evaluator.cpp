#include "expresssion_evaluator.h"
#include "value_ops.h"

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

void ExpressionEvaluator::VisitCXXMemberCallExpr(const clang::CXXMemberCallExpr* expr)
{
    VisitorScope vScope(this, "VisitCXXMemberCallExpr");
    const clang::CXXRecordDecl* rec = expr->getRecordDecl();
    const clang::CXXMethodDecl* method = expr->getMethodDecl();

    std::string recName = rec->getQualifiedNameAsString();
    std::cout << "[ExpressionEvaluator] Call method '" << method->getNameAsString() << "' from '" << recName << "'" << std::endl;

    Value object;
    std::vector<Value> args;
    Value result;
    if (EvalSubexpr(expr->getImplicitObjectArgument(), object) && CalculateCallArgs(expr->getArgs(), expr->getNumArgs(), args))
    {
        m_evalResult = value_ops::CallMember(m_interpreter, object, method, args, result);
    }

    vScope.Submit(std::move(result));
}

void ExpressionEvaluator::VisitCXXConstructExpr(const clang::CXXConstructExpr* expr)
{
    VisitorScope vScope(this, "VisitCXXConstructExpr");
    Value result;
    auto ctorDecl = expr->getConstructor();
    if (ctorDecl->isCopyConstructor() || ctorDecl->isMoveConstructor())
    {
        std::vector<Value> args;
        Value result;
        if (!CalculateCallArgs(expr->getArgs(), expr->getNumArgs(), args))
            return;

        if (ctorDecl->isCopyConstructor())
        {
            result = args[0];
        }
        else
        {
            result.AssignValue(std::move(args[0]));
        }

    }
    else
    {
        std::cout << "!!!!! Unsupported (yet!) type of ctor!" << std::endl;
        m_evalResult = false;
        return;
    }
    vScope.Submit(std::move(result));
}

void ExpressionEvaluator::VisitDeclRefExpr(const clang::DeclRefExpr* expr)
{
    VisitorScope vScope(this, "VisitDeclRefExpr");
    std::cout << "[ExpressionEvaluator] Declaration reference found: '" << expr->getNameInfo().getAsString() << "'" << std::endl;

    auto res = m_interpreter->GetDeclReference(expr->getFoundDecl());
    if (!res)
    {
        ReportError(expr->getLocation(), res.error());
        return;
    }

    vScope.Submit(std::move(res.value()));
}

void ExpressionEvaluator::VisitImplicitCastExpr(const clang::ImplicitCastExpr* expr)
{
    VisitorScope vScope(this, "VisitImplicitCastExpr");
    Value srcVal;
    if (!EvalSubexpr(expr->getSubExpr(), srcVal))
        return;

    vScope.Submit(std::move(srcVal));
}

void ExpressionEvaluator::VisitStringLiteral(const clang::StringLiteral* expr)
{
    VisitorScope vScope(this, "VisitStringLiteral");
    Value val(expr->getString().str());

    std::cout << "String literal found: '" << expr->getString().str() << "'" << std::endl;

    vScope.Submit(std::move(val));
}

void ExpressionEvaluator::VisitExprWithCleanups(const clang::ExprWithCleanups* expr)
{
    VisitorScope vScope(this, "VisitExprWithCleanups");
    Value val;

    if (!EvalSubexpr(expr->getSubExpr(), val))
        return;

    vScope.Submit(std::move(val));
}

void ExpressionEvaluator::VisitMaterializeTemporaryExpr(const clang::MaterializeTemporaryExpr* expr)
{
    VisitorScope vScope(this, "VisitMaterializeTemporaryExpr");

    Value val;

    if (!EvalSubexpr(expr->GetTemporaryExpr(), val))
        return;

    vScope.Submit(std::move(val));
}

bool ExpressionEvaluator::EvalSubexpr(const Expr* expr, Value& val)
{
    auto prevVal = m_currentValue;
    m_currentValue = &val;
    Visit(expr);
    m_currentValue = prevVal;
    return m_evalResult;
}

bool ExpressionEvaluator::CalculateCallArgs(const Expr* const* args, unsigned numArgs, std::vector<Value>& argValues)
{
    for (unsigned argNum = 0; argNum < numArgs; ++ argNum)
    {
        auto& arg = args[argNum];
        std::cout << "[ExpressionEvaluator] Enter evaluation of arg #" << argNum << "'" << std::endl;
        Value val;
        if (!EvalSubexpr(arg, val))
            return false;
        argValues.push_back(std::move(val));
        std::cout << "[ExpressionEvaluator] Exit evaluation of arg #" << argNum << "'" << std::endl;
    }

    return true;
}

void ExpressionEvaluator::ReportError(const SourceLocation& loc, const std::string& errMsg)
{
    m_interpreter->Report(Diag::Error, loc, errMsg);
    m_evalResult = false;
}


} // interpreter
} // codegen

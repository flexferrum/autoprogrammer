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
        if (!CalculateCallArgs(expr->getArgs(), expr->getNumArgs(), args))
            return;

        std::cout << "[ExpressionEvaluator] Constructor call. Arg0 type: " << args[0].GetValue().which() << std::endl;
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

void ExpressionEvaluator::VisitCXXOperatorCallExpr(const clang::CXXOperatorCallExpr* expr)
{
    VisitorScope vScope(this, "VisitCXXOperatorCallExpr");
    std::cout << "[ExpressionEvaluator] Overloaded operator call found. Num args: " << expr->getNumArgs() << ", DeclKind: '" << expr->getCalleeDecl()->getDeclKindName() << "'" << std::endl;

    const clang::Decl* calleeDecl = expr->getCalleeDecl();
    const clang::CXXMethodDecl* operAsMethod = llvm::dyn_cast_or_null<CXXMethodDecl>(calleeDecl);

    std::vector<Value> args;
    Value result;
    if (!CalculateCallArgs(expr->getArgs(), expr->getNumArgs(), args))
        return;

    if (operAsMethod != nullptr)
    {
        std::vector<Value> newArgs(std::make_move_iterator(args.begin() + 1), std::make_move_iterator(args.end()));
        m_evalResult = value_ops::CallMember(m_interpreter, args[0], operAsMethod, newArgs, result);
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

void ExpressionEvaluator::VisitBinaryOperator(const clang::BinaryOperator* expr)
{
    VisitorScope vScope(this, "VisitBinaryOperator");

    std::cout << "[ExpressionEvaluator] Binary operator found: '" << expr->getOpcodeStr().str() << "'" << std::endl;

    Value result;

    if (expr->isLogicalOp())
    {
        Value lhs;
        Value rhs;
        if (!EvalSubexpr(expr->getLHS(), lhs))
            return;

        bool leftBool = value_ops::ConvertToBool(m_interpreter, lhs);

        if (expr->getOpcode() == BO_LAnd)
        {
            if (leftBool)
            {
                EvalSubexpr(expr->getRHS(), rhs);
                result = Value(value_ops::ConvertToBool(m_interpreter, rhs));
            }
            else
            {
                result = Value(false);
            }
        }
        else // BO_LOr
        {
            if (!leftBool)
            {
                EvalSubexpr(expr->getRHS(), rhs);
                result = Value(value_ops::ConvertToBool(m_interpreter, rhs));
            }
            else
            {
                result = Value(true);
            }
        }
    }
    else
    {
        assert(false); // TODO: !!!!
    }

    vScope.Submit(std::move(result));
}

void ExpressionEvaluator::VisitUnaryOperator(const clang::UnaryOperator* expr)
{
    VisitorScope vScope(this, "VisitUnaryOperator");

    std::cout << "[ExpressionEvaluator] Unary operator found: '" << UnaryOperator::getOpcodeStr(expr->getOpcode()).str() << "'" << std::endl;
    auto kind = expr->getOpcode();

    Value result;
    Value val;
    if (!EvalSubexpr(expr->getSubExpr(), val))
        return;

    if (kind == UO_LNot)
    {
        result = Value(!value_ops::ConvertToBool(m_interpreter, val));
    }
    else
    {
        assert(false); // TODO: !!!
    }

    vScope.Submit(std::move(result));
}

void ExpressionEvaluator::VisitParenExpr(const ParenExpr* expr)
{
    VisitorScope vScope(this, "VisitParenExpr");

    Value val;

    if (!EvalSubexpr(expr->getSubExpr(), val))
        return;

    vScope.Submit(std::move(val));
    // std::cout << "[ExpressionEvaluator] Unary operator found: '" << UnaryOperator::getOpcodeStr(expr->getOpcode()).str() << "'" << std::endl;

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

#include "expresssion_evaluator.h"

#include "../../type_info.h"
#include "reflected_range.h"
#include "value_ops.h"

#include <clang/AST/Attr.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/TemplateBase.h>

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
    dbg() << "[ExpressionEvaluator] Call method '" << method->getNameAsString() << "' from '" << recName << "'" << std::endl;

    Value object;
    std::vector<Value> args;
    Value result;
    if (EvalSubexpr(expr->getImplicitObjectArgument(), object) && CalculateCallArgs(expr->getArgs(), expr->getNumArgs(), args))
    {
        m_evalResult = value_ops::CallMember(m_interpreter, object, method, args, result);
    }

    vScope.Submit(std::move(result));
}

void ExtractTemplateTypeArgs(const ArrayRef<TemplateArgument>& args, std::vector<reflection::TypeInfoPtr>& types, const clang::ASTContext* astContext)
{
    for (const clang::TemplateArgument& tplArg : args)
    {
        auto argKind = tplArg.getKind();
        if (argKind == TemplateArgument::Pack)
            ExtractTemplateTypeArgs(tplArg.getPackAsArray(), types, astContext);
        else if (argKind == TemplateArgument::Type)
        {
            auto typeInfo = reflection::TypeInfo::Create(tplArg.getAsType(), astContext);
            types.push_back(typeInfo);
        }
    }
}

void ExpressionEvaluator::VisitCallExpr(const CallExpr* expr)
{
    VisitorScope vScope(this, "VisitCallExpr");

    const clang::FunctionDecl* function = expr->getDirectCallee();
    m_evalResult = false;

    if (!function)
        return;

    std::string fnName = function->getQualifiedNameAsString();
    dbg() << "[ExpressionEvaluator] Call function '" << fnName << "'" << std::endl;

    Value result;
    if (fnName == "meta::reflect_type")
    {
        const clang::TemplateArgumentList* tplArgs = function->getTemplateSpecializationArgs();
        if (tplArgs == nullptr)
        {
            dbg() << "[ExpressionEvaluator] Template arguments list empty for meta::reflect_type" << std::endl;
            return;
        }

        std::vector<reflection::TypeInfoPtr> types;
        ExtractTemplateTypeArgs(tplArgs->asArray(), types, m_interpreter->m_astContext);
        dbg() << "[ExpressionEvaluator] Template arguments:" << std::endl;
        for (auto& arg : types)
            dbg() << "[ExpressionEvaluator] \t" << arg << std::endl;

        result = Value(ReflectedObject(types[0]));
        m_evalResult = true;
        vScope.Submit(std::move(result));
    }
    if (fnName == "meta::reflect_type_list")
    {
        const clang::TemplateArgumentList* tplArgs = function->getTemplateSpecializationArgs();
        if (tplArgs == nullptr)
        {
            dbg() << "[ExpressionEvaluator] Template arguments list empty for meta::reflect_type" << std::endl;
            return;
        }

        std::vector<reflection::TypeInfoPtr> types;
        ExtractTemplateTypeArgs(tplArgs->asArray(), types, m_interpreter->m_astContext);
        dbg() << "[ExpressionEvaluator] Template arguments:" << std::endl;
        for (auto& arg : types)
            dbg() << "[ExpressionEvaluator] \t" << arg << std::endl;

        auto range = MakeStdCollectionRefRange(std::move(types));
        result = Value(ReflectedObject(range));
        m_evalResult = true;
        vScope.Submit(std::move(result));
    }
    else if (fnName == "meta::project_type")
    {
        std::vector<Value> args;
        Value result;
        if (CalculateCallArgs(expr->getArgs(), expr->getNumArgs(), args))
        {
            result = args[0];
            m_evalResult = true;
            vScope.Submit(std::move(result));
        }
    }

    // function->dump();
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

        dbg() << "[ExpressionEvaluator] Constructor call. Arg0 type: " << args[0].GetValue().which() << std::endl;
        if (ctorDecl->isCopyConstructor())
        {
            result = args[0];
        }
        else
        {
            result.AssignValue(std::move(args[0]));
        }

        m_evalResult = true;
    }
    else
    {
        std::vector<Value> args;
        if (!CalculateCallArgs(expr->getArgs(), expr->getNumArgs(), args))
            return;

        dbg() << "[ExpressionEvaluator] Constructor call." << std::endl;

        m_evalResult = value_ops::CallFunction(m_interpreter, ctorDecl, args, result);
    }
    vScope.Submit(std::move(result));
}

void ExpressionEvaluator::VisitCXXOperatorCallExpr(const clang::CXXOperatorCallExpr* expr)
{
    VisitorScope vScope(this, "VisitCXXOperatorCallExpr");
    dbg() << "[ExpressionEvaluator] Overloaded operator call found. Num args: " << expr->getNumArgs() << ", DeclKind: '"
          << expr->getCalleeDecl()->getDeclKindName() << "'" << std::endl;

    const clang::Decl* calleeDecl = expr->getCalleeDecl();
    const clang::CXXMethodDecl* operAsMethod = llvm::dyn_cast_or_null<CXXMethodDecl>(calleeDecl);
    const clang::FunctionDecl* operAsFunction = llvm::dyn_cast_or_null<FunctionDecl>(calleeDecl);

    std::vector<Value> args;
    Value result;
    if (!CalculateCallArgs(expr->getArgs(), expr->getNumArgs(), args))
        return;

    if (operAsMethod != nullptr)
    {
        std::vector<Value> newArgs(std::make_move_iterator(args.begin() + 1), std::make_move_iterator(args.end()));
        m_evalResult = value_ops::CallMember(m_interpreter, args[0], operAsMethod, newArgs, result);
        if (m_evalResult && expr->isAssignmentOp())
            ReplaceByReference(args[0], result);
    }
    else
    {
        m_evalResult = value_ops::CallFunction(m_interpreter, operAsFunction, args, result);
    }

    vScope.Submit(std::move(result));
}

void ExpressionEvaluator::VisitDeclRefExpr(const clang::DeclRefExpr* expr)
{
    VisitorScope vScope(this, "VisitDeclRefExpr");
    dbg() << "[ExpressionEvaluator] Declaration reference found: '" << expr->getNameInfo().getAsString() << "'" << std::endl;

    auto res = m_interpreter->GetDeclReference(expr->getFoundDecl());
    if (!res)
    {
        ReportError(expr->getLocation(), res.error());
        return;
    }

    m_evalResult = true;
    vScope.Submit(std::move(res.value()));
}

void ExpressionEvaluator::VisitImplicitCastExpr(const clang::ImplicitCastExpr* expr)
{
    VisitorScope vScope(this, "VisitImplicitCastExpr");
    Value srcVal;
    if (!EvalSubexpr(expr->getSubExpr(), srcVal))
        return;

    m_evalResult = true;
    vScope.Submit(std::move(srcVal));
}

void ExpressionEvaluator::VisitStringLiteral(const clang::StringLiteral* expr)
{
    VisitorScope vScope(this, "VisitStringLiteral");
    Value val(expr->getString().str());

    dbg() << "String literal found: '" << expr->getString().str() << "'" << std::endl;

    m_evalResult = true;
    vScope.Submit(std::move(val));
}

void ExpressionEvaluator::VisitCXXBoolLiteralExpr(const clang::CXXBoolLiteralExpr* expr)
{
    VisitorScope vScope(this, "VisitCXXBoolLiteralExpr");
    Value val(expr->getValue());

    dbg() << "Bool literal found: '" << (expr->getValue() ? "true" : "false") << "'" << std::endl;

    m_evalResult = true;
    vScope.Submit(std::move(val));
}

void ExpressionEvaluator::VisitExprWithCleanups(const clang::ExprWithCleanups* expr)
{
    VisitorScope vScope(this, "VisitExprWithCleanups");
    Value val;

    if (!EvalSubexpr(expr->getSubExpr(), val))
        return;

    m_evalResult = true;
    vScope.Submit(std::move(val));
}

void ExpressionEvaluator::VisitMaterializeTemporaryExpr(const clang::MaterializeTemporaryExpr* expr)
{
    VisitorScope vScope(this, "VisitMaterializeTemporaryExpr");

    Value val;

    if (!EvalSubexpr(expr->GetTemporaryExpr(), val))
        return;

    m_evalResult = true;
    vScope.Submit(std::move(val));
}

void ExpressionEvaluator::VisitCXXBindTemporaryExpr(const clang::CXXBindTemporaryExpr* expr)
{
    VisitorScope vScope(this, "VisitCXXBindTemporaryExpr");

    Value val;

    if (!EvalSubexpr(expr->getSubExpr(), val))
        return;

    m_evalResult = true;
    vScope.Submit(std::move(val));
}

void ExpressionEvaluator::VisitBinaryOperator(const clang::BinaryOperator* expr)
{
    VisitorScope vScope(this, "VisitBinaryOperator");

    dbg() << "[ExpressionEvaluator] Binary operator found: '" << expr->getOpcodeStr().str() << "'" << std::endl;

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

    m_evalResult = true;
    vScope.Submit(std::move(result));
}

void ExpressionEvaluator::VisitUnaryOperator(const clang::UnaryOperator* expr)
{
    VisitorScope vScope(this, "VisitUnaryOperator");

    dbg() << "[ExpressionEvaluator] Unary operator found: '" << UnaryOperator::getOpcodeStr(expr->getOpcode()).str() << "'" << std::endl;
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

    m_evalResult = true;
    vScope.Submit(std::move(result));
}

void ExpressionEvaluator::VisitParenExpr(const ParenExpr* expr)
{
    VisitorScope vScope(this, "VisitParenExpr");

    Value val;

    if (!EvalSubexpr(expr->getSubExpr(), val))
        return;

    m_evalResult = true;
    vScope.Submit(std::move(val));
    // dbg() << "[ExpressionEvaluator] Unary operator found: '" << UnaryOperator::getOpcodeStr(expr->getOpcode()).str() << "'" << std::endl;
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
    for (unsigned argNum = 0; argNum < numArgs; ++argNum)
    {
        auto& arg = args[argNum];
        dbg() << "[ExpressionEvaluator] Enter evaluation of arg #" << argNum << "'" << std::endl;
        Value val;
        if (!EvalSubexpr(arg, val))
            return false;
        argValues.push_back(std::move(val));
        dbg() << "[ExpressionEvaluator] Exit evaluation of arg #" << argNum << "'" << std::endl;
    }

    return true;
}

void ExpressionEvaluator::ReplaceByReference(Value& origValue, const Value& newValue)
{
    auto& val = origValue.GetValue();
    Value* ptr = nullptr;

    auto ptrCase = boost::get<Value::Pointer>(&val);
    if (ptrCase != nullptr)
        ptr = ptrCase->pointee;
    else
    {
        auto intRefCase = boost::get<Value::InternalRef>(&val);
        if (intRefCase != nullptr)
            ptr = intRefCase->pointee;
        else
        {
            auto refCase = boost::get<Value::Reference>(&val);
            if (refCase != nullptr)
                ptr = refCase->pointee;
        }
    }

    if (ptr != nullptr)
        ptr->AssignValue(newValue);
}

void ExpressionEvaluator::ReportError(const SourceLocation& loc, const std::string& errMsg)
{
    m_interpreter->Report(MessageType::Error, loc, errMsg);
    m_evalResult = false;
}

} // interpreter
} // codegen

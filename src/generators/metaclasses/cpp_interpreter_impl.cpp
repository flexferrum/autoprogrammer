// Code in this file partially taken from clang 'lib/AST/ExprConstant.cpp' file

#include "cpp_interpreter.h"
#include "cpp_interpreter_impl.h"
#include "expresssion_evaluator.h"

#include "type_info.h"
#include "value.h"
#include "value_ops.h"
#include "reflected_object.h"

#include <clang/AST/Decl.h>

using namespace clang;
using namespace reflection;

namespace codegen
{
namespace interpreter
{

void InterpreterImpl::ExecuteMethod(const CXXMethodDecl* method)
{
    Stmt* body = method->getBody();
    if (body == nullptr)
        return;

    BlockScopeRAII scope(m_scopes);
    ExecuteStatement(body, nullptr);
}

InterpreterImpl::ExecStatementResult InterpreterImpl::ExecuteStatement(const Stmt* stmt, const SwitchCase* curSwithCase)
{
    std::cout << "<><><><><><><><><> Executing statement: " << stmt->getStmtClassName() << std::endl;

    switch (stmt->getStmtClass())
    {
    case Stmt::NullStmtClass:
        return ESR_Succeeded;

    case Stmt::CompoundStmtClass:
    {
        BlockScopeRAII scope(m_scopes);

        const CompoundStmt* s = cast<CompoundStmt>(stmt);
        for (const auto* item : s->body())
        {
            auto result = ExecuteStatement(item, curSwithCase);
            if (result == ESR_Succeeded)
                curSwithCase = nullptr;
            else if (result != ESR_CaseNotFound)
                return result;
        }
        return curSwithCase ? ESR_CaseNotFound : ESR_Succeeded;
    }

    case Stmt::CXXForRangeStmtClass:
    {
        const CXXForRangeStmt *forStmt = cast<CXXForRangeStmt>(stmt);

        BlockScopeRAII scope(m_scopes);

        // Initialize the __range variable.
        ExecStatementResult result = ExecuteStatement(forStmt->getRangeStmt(), nullptr);
        if (result != ESR_Succeeded)
            return result;

        // Create the __begin and __end iterators.
        result = ExecuteStatement(forStmt->getBeginStmt(), nullptr);
        if (result != ESR_Succeeded)
            return result;
        result = ExecuteStatement(forStmt->getEndStmt(), nullptr);
        if (result != ESR_Succeeded)
            return result;

        while (true)
        {
            // Condition: __begin != __end.
            {
                bool doContinue = true;
                ExprScopeRAII condExpr(m_scopes);
                if (!ExecuteAsBooleanCondition(forStmt->getCond(), doContinue))
                    return ESR_Failed;
                if (!doContinue)
                    break;
            }

            // User's variable declaration, initialized by *__begin.
            BlockScopeRAII innerScope(m_scopes);
            result = ExecuteStatement(forStmt->getLoopVarStmt(), nullptr);
            if (result != ESR_Succeeded)
                return result;

            // Loop body.
            result = EvaluateLoopBody(forStmt->getBody(), nullptr);
            if (result != ESR_Continue)
                return result;

            // Increment: ++__begin
            Value ignoredResult;
            if (!ExecuteExpression(forStmt->getInc(), ignoredResult))
                return ESR_Failed;
        }

        return ESR_Succeeded;
    }

    case Stmt::DeclStmtClass:
    {
        const clang::DeclStmt *declStmt = cast<DeclStmt>(stmt);
        for (const auto *decl : declStmt->decls())
        {
            // Each declaration initialization is its own full-expression.
            // FIXME: This isn't quite right; if we're performing aggregate
            // initialization, each braced subexpression is its own full-expression.
            ExprScopeRAII scope(m_scopes);
            if (!ExecuteDecl(decl))
                return ESR_Failed;
        }
        return ESR_Succeeded;
    }

    case Stmt::IfStmtClass:
    {
      const IfStmt *ifStmt = cast<IfStmt>(stmt);

      // Evaluate the condition, as either a var decl or as an expression.
      BlockScopeRAII scope(m_scopes);
      if (const Stmt *initStmt = ifStmt->getInit())
      {
        auto result = ExecuteStatement(initStmt, nullptr);

        if (result != ESR_Succeeded)
          return result;
      }
      bool cond = false;
      auto condVarStmt = ifStmt->getConditionVariable();
      if (condVarStmt && !ExecuteVarDecl(condVarStmt))
          return ESR_Failed;

      if (!ExecuteAsBooleanCondition(ifStmt->getCond(), cond))
        return ESR_Failed;

      if (const Stmt *subStmt = cond ? ifStmt->getThen() : ifStmt->getElse())
      {
        auto result = ExecuteStatement(subStmt, nullptr);
        if (result != ESR_Succeeded)
          return result;
      }
      return ESR_Succeeded;
    }

    default:
        if (const Expr *expr = dyn_cast<Expr>(stmt))
        {
            ExprScopeRAII scope(m_scopes);
            Value result;
            if (!ExecuteExpression(expr, result))
                return ESR_Failed;
            return ESR_Succeeded;
        }

        std::cout << "<><><><><><><><><> Unknown statement: " << stmt->getStmtClassName() << std::endl;
        return ESR_Failed;
#if 0
    case Stmt::DeclStmtClass: {
      const DeclStmt *DS = cast<DeclStmt>(S);
      for (const auto *DclIt : DS->decls()) {
        // Each declaration initialization is its own full-expression.
        // FIXME: This isn't quite right; if we're performing aggregate
        // initialization, each braced subexpression is its own full-expression.
        FullExpressionRAII Scope(Info);
        if (!EvaluateDecl(Info, DclIt) && !Info.noteFailure())
          return ESR_Failed;
      }
      return ESR_Succeeded;
    }

    case Stmt::ReturnStmtClass: {
      const Expr *RetExpr = cast<ReturnStmt>(S)->getRetValue();
      FullExpressionRAII Scope(Info);
      if (RetExpr &&
          !(Result.Slot
                ? EvaluateInPlace(Result.Value, Info, *Result.Slot, RetExpr)
                : Evaluate(Result.Value, Info, RetExpr)))
        return ESR_Failed;
      return ESR_Returned;
    }

    case Stmt::WhileStmtClass: {
      const WhileStmt *WS = cast<WhileStmt>(S);
      while (true) {
        BlockScopeRAII Scope(Info);
        bool Continue;
        if (!EvaluateCond(Info, WS->getConditionVariable(), WS->getCond(),
                          Continue))
          return ESR_Failed;
        if (!Continue)
          break;

        EvalStmtResult ESR = EvaluateLoopBody(Result, Info, WS->getBody());
        if (ESR != ESR_Continue)
          return ESR;
      }
      return ESR_Succeeded;
    }

    case Stmt::DoStmtClass: {
      const DoStmt *DS = cast<DoStmt>(S);
      bool Continue;
      do {
        EvalStmtResult ESR = EvaluateLoopBody(Result, Info, DS->getBody(), Case);
        if (ESR != ESR_Continue)
          return ESR;
        Case = nullptr;

        FullExpressionRAII CondScope(Info);
        if (!EvaluateAsBooleanCondition(DS->getCond(), Continue, Info))
          return ESR_Failed;
      } while (Continue);
      return ESR_Succeeded;
    }

    case Stmt::ForStmtClass: {
      const ForStmt *FS = cast<ForStmt>(S);
      BlockScopeRAII Scope(Info);
      if (FS->getInit()) {
        EvalStmtResult ESR = EvaluateStmt(Result, Info, FS->getInit());
        if (ESR != ESR_Succeeded)
          return ESR;
      }
      while (true) {
        BlockScopeRAII Scope(Info);
        bool Continue = true;
        if (FS->getCond() && !EvaluateCond(Info, FS->getConditionVariable(),
                                           FS->getCond(), Continue))
          return ESR_Failed;
        if (!Continue)
          break;

        EvalStmtResult ESR = EvaluateLoopBody(Result, Info, FS->getBody());
        if (ESR != ESR_Continue)
          return ESR;

        if (FS->getInc()) {
          FullExpressionRAII IncScope(Info);
          if (!EvaluateIgnoredValue(Info, FS->getInc()))
            return ESR_Failed;
        }
      }
      return ESR_Succeeded;
    }

    case Stmt::SwitchStmtClass:
      return EvaluateSwitch(Result, Info, cast<SwitchStmt>(S));

    case Stmt::ContinueStmtClass:
      return ESR_Continue;

    case Stmt::BreakStmtClass:
      return ESR_Break;

    case Stmt::LabelStmtClass:
      return EvaluateStmt(Result, Info, cast<LabelStmt>(S)->getSubStmt(), Case);

    case Stmt::AttributedStmtClass:
      // As a general principle, C++11 attributes can be ignored without
      // any semantic impact.
      return EvaluateStmt(Result, Info, cast<AttributedStmt>(S)->getSubStmt(),
                          Case);

    case Stmt::CaseStmtClass:
    case Stmt::DefaultStmtClass:
      return EvaluateStmt(Result, Info, cast<SwitchCase>(S)->getSubStmt(), Case);
    }
#endif
}
}

InterpreterImpl::ExecStatementResult InterpreterImpl::EvaluateLoopBody(const clang::Stmt* body, const SwitchCase* curSwitchCase) {
    BlockScopeRAII scope(m_scopes);
    switch (auto result = ExecuteStatement(body, curSwitchCase))
    {
    case ESR_Break:
        return ESR_Succeeded;
    case ESR_Succeeded:
    case ESR_Continue:
        return ESR_Continue;
    case ESR_Failed:
    case ESR_Returned:
    case ESR_CaseNotFound:
        return result;
    }
    llvm_unreachable("Invalid EvalStmtResult!");
}


bool InterpreterImpl::ExecuteExpression(const Expr* expr, Value& result)
{
    expr->dump();
    bool isOk = true;
    ExpressionEvaluator evaluator(this, result, isOk);

    evaluator.Visit(expr);

    return isOk;
#if 0
    QualType exprType = expr->getType();
    if (expr->isGLValue() || exprType->isFunctionType())
    {
        std::cout << "<><><><><><><><> Evaluate expression as LValue or function" << std::endl;
//        LValue value;
//        if (!EvaluateLValue(expr, LV, Info))
//            return false;
//        LV.moveInto(Result);
    }
    else if (exprType->isIntegralOrEnumerationType())
    {
        std::cout << "<><><><><><><><> Evaluate expression as kind of integer" << std::endl;
//        if (!IntExprEvaluator(Info, Result).Visit(expr))
//            return false;
    }
    else if (exprType->hasPointerRepresentation())
    {
        std::cout << "<><><><><><><><> Evaluate expression as kind of pointer" << std::endl;
//        LValue LV;
//        if (!EvaluatePointer(expr, LV, Info))
//            return false;
//        LV.moveInto(Result);
    }
    else if (exprType->isRealFloatingType())
    {
        std::cout << "<><><><><><><><> Evaluate expression as float value" << std::endl;
//        llvm::APFloat F(0.0);
//        if (!EvaluateFloat(expr, F, Info))
//            return false;
//        Result = APValue(F);
    }
    else if (exprType->isMemberPointerType())
    {
        std::cout << "<><><><><><><><> Evaluate expression as pointer to member" << std::endl;
//        MemberPtr P;
//        if (!EvaluateMemberPointer(expr, P, Info))
//            return false;
//        P.moveInto(Result);
        return true;
    }
    else if (exprType->isArrayType())
    {
        std::cout << "<><><><><><><><> Evaluate expression as array" << std::endl;
//        LValue LV;
//        LV.set(expr, Info.CurrentCall->Index);
//        APValue &Value = Info.CurrentCall->createTemporary(expr, false);
//        if (!EvaluateArray(expr, LV, Value, Info))
//            return false;
//        Result = Value;
    }
    else if (exprType->isRecordType())
    {
        std::cout << "<><><><><><><><> Evaluate expression as record type" << std::endl;
//        LValue LV;
//        LV.set(expr, Info.CurrentCall->Index);
//        APValue &Value = Info.CurrentCall->createTemporary(expr, false);
//        if (!EvaluateRecord(expr, LV, Value, Info))
//            return false;
//        Result = Value;
    }
    else if (exprType->isVoidType())
    {
        std::cout << "<><><><><><><><> Evaluate expression as void type" << std::endl;
//        if (!Info.getLangOpts().CPlusPlus11)
//            Info.CCEDiag(expr, diag::note_constexpr_nonliteral)
//                    << expr->getType();
//        if (!EvaluateVoid(expr, Info))
//            return false;
    }
    else
    {
        std::cout << "<><><><><><><><> Unsupported expression type: " << exprType->getTypeClassName() << std::endl;
//        Info.FFDiag(expr, diag::note_invalid_subexpr_in_const_expr);
        return false;
    }
#endif
}

bool InterpreterImpl::ExecuteAsBooleanCondition(const Expr* expr, bool& result)
{
    Value val;
    if (!ExecuteExpression(expr, val))
        return false;

    result = value_ops::ConvertToBool(this, val);
    return true;
}

bool InterpreterImpl::ExecuteVarDecl(const VarDecl* decl)
{
    // TODO: Handle static variables (but how???)
    if (!decl->hasLocalStorage())
        return true;

    ScopeStack::DeclInfo* localVar = CreateLocalVar(decl);

    const Expr *initExpr = decl->getInit();
    if (!initExpr)
    {
        assert(false);
    }
    else
    {
        Value initVal;
        if (!ExecuteExpression(initExpr, initVal))
            return false;
        localVar->val.AssignValue(std::move(initVal));
    }

    return true;
}

ScopeStack::DeclInfo* InterpreterImpl::CreateLocalVar(const VarDecl* decl)
{
    ScopeStack::DeclInfo init;
    init.decl = cast<NamedDecl>(decl);
    init.isLifetimeExtended = true;

    m_scopes.stack.push_back(std::move(init));
    auto& result = m_scopes.stack.back();
    m_visibleDecls[init.decl] = Value::InternalRef(&result.val);
    std::cout << "Local variable created: " << decl->getNameAsString() << std::endl;

    return &result;
}

bool InterpreterImpl::ExecuteDecl(const Decl* D)
{
    bool OK = true;

    if (const VarDecl *VD = dyn_cast<VarDecl>(D))
        OK &= ExecuteVarDecl(VD);

#if 0
    if (const DecompositionDecl *DD = dyn_cast<DecompositionDecl>(D))
        for (auto *BD : DD->bindings())
            if (auto *VD = BD->getHoldingVar())
                OK &= EvaluateDecl(Info, VD);
#endif
    return OK;
}


nonstd::expected<Value, std::string> InterpreterImpl::GetDeclReference(const clang::NamedDecl* decl)
{
    auto p = m_visibleDecls.find(decl);
    if (p != m_visibleDecls.end())
    {
        std::cout << "Variable resolved: '" + decl->getNameAsString() + "'" << std::endl;
        return Value(p->second);
    }

    Value val;
    if (DetectSpecialDecl(decl, val))
        return val;

    return nonstd::make_unexpected("Can't resolve variable '" + decl->getNameAsString() + "'");
}

bool InterpreterImpl::DetectSpecialDecl(const clang::NamedDecl* decl, Value& val)
{
    const clang::VarDecl* varDecl = llvm::dyn_cast_or_null<const clang::VarDecl>(decl);

    if (!varDecl)
        return false;

    reflection::TypeInfoPtr ti = reflection::TypeInfo::Create(varDecl->getType(), m_astContext);
    std::string varName = varDecl->getNameAsString();
    std::string valueKey = varName + ":>" + ti->getFullQualifiedName();
    Value* actual = &m_globalVars[valueKey];
    if (valueKey == "compiler:>meta::CompilerImpl" && ti->getIsReference())
    {
        *actual = ReflectedObject(Compiler());
    }
    else if(varName.length() >= 1 && varName[0] == '$')
    {
        std::string tmpName = "MetaClass_" + varName.substr(1);
        std::cout << "Possible reference to metaclass found: " << tmpName << std::endl;
        const DeclContext* ctx = decl->getDeclContext();
        if (ctx->isRecord())
        {
            auto rec = llvm::dyn_cast_or_null<const clang::RecordDecl>(ctx);
            // assert(rec);
            std::string name = rec->getNameAsString();
            if (name == tmpName)
                *actual = ReflectedObject(m_instance);
        }
    }

    if (!actual->IsEmpty())
    {
        val = Value::InternalRef(actual);
        m_visibleDecls[decl] = Value::InternalRef(actual);
    }
    else
        return false;

    std::cout << "Found reference to variable declaration. Variable name: " << varDecl->getNameAsString() << "', variable type: '" << ti->getFullQualifiedName() << "'" << std::endl;
    return true;
}

bool InterpreterImpl::Report(Diag type, const clang::SourceLocation& loc, std::string message)
{
    return !(type == Diag::Error);
}

PrintingPolicy InterpreterImpl::GetDefaultPrintingPolicy()
{
    PrintingPolicy pp(m_astContext->getLangOpts());
    pp.Bool = true;
    return pp;
}
} // interpreter

CppInterpreter::CppInterpreter(const clang::ASTContext* astContext)
    : m_astContext(astContext)
{

}

CppInterpreter::~CppInterpreter()
{
}

void CppInterpreter::Execute(reflection::ClassInfoPtr metaclass, reflection::ClassInfoPtr inst, reflection::MethodInfoPtr generator)
{
    interpreter::InterpreterImpl i(m_astContext, metaclass, inst);
    i.ExecuteMethod(generator->decl);
}

} // codegen

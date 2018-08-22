// Code in this file partially taken from clang 'lib/AST/ExprConstant.cpp' file

#include "cpp_interpreter.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/StmtCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/APValue.h>

using namespace clang;
using namespace reflection;

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

struct LValue
{
    APValue val;
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

    void ExecuteMethod(const CXXMethodDecl* method);

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
    ExecStatementResult ExecuteStatement(const Stmt* stmt, const SwitchCase* curSwithCase);
    bool ExecuteExpression(const Expr* expr, APValue& result);

private:
    using BlockScopeRAII = RAIIScope<false>;
    using ExprScopeRAII = RAIIScope<true>;

    const clang::ASTContext* m_astContext;
    reflection::ClassInfoPtr m_metaclass;
    reflection::ClassInfoPtr m_instance;
    std::stack<ScopeInfo> m_scopes;
};

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

    default:
        if (const Expr *expr = dyn_cast<Expr>(stmt))
        {
            ExprScopeRAII scope(m_scopes);
            APValue result;
            if (!ExecuteExpression(expr, result))
                return ESR_Failed;
            return ESR_Succeeded;
        }

        std::cout << "<><><><><><><><><> Executing statement: " << stmt->getStmtClassName() << std::endl;
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

    case Stmt::IfStmtClass: {
      const IfStmt *IS = cast<IfStmt>(S);

      // Evaluate the condition, as either a var decl or as an expression.
      BlockScopeRAII Scope(Info);
      if (const Stmt *Init = IS->getInit()) {
        EvalStmtResult ESR = EvaluateStmt(Result, Info, Init);
        if (ESR != ESR_Succeeded)
          return ESR;
      }
      bool Cond;
      if (!EvaluateCond(Info, IS->getConditionVariable(), IS->getCond(), Cond))
        return ESR_Failed;

      if (const Stmt *SubStmt = Cond ? IS->getThen() : IS->getElse()) {
        EvalStmtResult ESR = EvaluateStmt(Result, Info, SubStmt);
        if (ESR != ESR_Succeeded)
          return ESR;
      }
      return ESR_Succeeded;
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

    case Stmt::CXXForRangeStmtClass: {
      const CXXForRangeStmt *FS = cast<CXXForRangeStmt>(S);
      BlockScopeRAII Scope(Info);

      // Initialize the __range variable.
      EvalStmtResult ESR = EvaluateStmt(Result, Info, FS->getRangeStmt());
      if (ESR != ESR_Succeeded)
        return ESR;

      // Create the __begin and __end iterators.
      ESR = EvaluateStmt(Result, Info, FS->getBeginStmt());
      if (ESR != ESR_Succeeded)
        return ESR;
      ESR = EvaluateStmt(Result, Info, FS->getEndStmt());
      if (ESR != ESR_Succeeded)
        return ESR;

      while (true) {
        // Condition: __begin != __end.
        {
          bool Continue = true;
          FullExpressionRAII CondExpr(Info);
          if (!EvaluateAsBooleanCondition(FS->getCond(), Continue, Info))
            return ESR_Failed;
          if (!Continue)
            break;
        }

        // User's variable declaration, initialized by *__begin.
        BlockScopeRAII InnerScope(Info);
        ESR = EvaluateStmt(Result, Info, FS->getLoopVarStmt());
        if (ESR != ESR_Succeeded)
          return ESR;

        // Loop body.
        ESR = EvaluateLoopBody(Result, Info, FS->getBody());
        if (ESR != ESR_Continue)
          return ESR;

        // Increment: ++__begin
        if (!EvaluateIgnoredValue(Info, FS->getInc()))
          return ESR_Failed;
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

bool InterpreterImpl::ExecuteExpression(const Expr* expr, APValue& result)
{
    expr->dump();
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

    return true;
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

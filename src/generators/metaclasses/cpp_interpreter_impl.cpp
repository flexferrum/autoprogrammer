// Code in this file partially taken from clang 'lib/AST/ExprConstant.cpp' file

#include "cpp_interpreter.h"
#include "cpp_interpreter_impl.h"
#include "expresssion_evaluator.h"
#include "injected_code_renderer.h"

#include "type_info.h"
#include "value.h"
#include "value_ops.h"
#include "reflected_object.h"
#include "ast_reflector.h"

#include <clang/AST/Decl.h>
#include <clang/AST/Attr.h>
#include <clang/AST/DeclVisitor.h>

#include <sstream>

using namespace clang;
using namespace reflection;

namespace codegen
{
namespace interpreter
{

class StatementInjectionContext : public CodeInjectionContext
{
public:
    void InjectMethodDecl(reflection::MethodInfoPtr methodInfo, /*const std::string& target, */const std::string& visibility) override
    {
        assert(false);
        return;
    }
    void InjectCodeFragment(const std::string& fragment, /*const std::string& target, */const std::string& visibility) override
    {
        m_os << fragment;
    }
    void InjectObject(const ReflectedObject& object, const std::string& visibility) override
    {
        assert(false);
        return;
    }

    std::string GetRenderResult() const {return m_os.str();}

private:
    std::ostringstream m_os;
};

class DeclInjectionContext : public CodeInjectionContext
{
public:
    DeclInjectionContext(InterpreterImpl* interpreter, reflection::ClassInfoPtr targetClass)
        : m_interpreter(interpreter)
        , m_targetClass(targetClass)
    {}

    void InjectMethodDecl(reflection::MethodInfoPtr methodInfo, /*const std::string& target, */const std::string& visibility) override
    {
        m_interpreter->InjectMethodDecl(methodInfo, m_targetClass, visibility);
    }
    void InjectCodeFragment(const std::string& fragment, /*const std::string& target, */const std::string& visibility) override
    {
        m_interpreter->InjectCodeFragment(fragment, m_targetClass, visibility);
    }
    void InjectObject(const ReflectedObject& object, const std::string& visibility) override
    {
        m_interpreter->InjectObjectInst(object, m_targetClass, visibility);
    }

private:
    InterpreterImpl* m_interpreter;
    reflection::ClassInfoPtr m_targetClass;
};

void InterpreterImpl::ExecuteMetaclassMethod(const FunctionDecl* method)
{
    Stmt* body = method->getBody();
    if (body == nullptr)
    {
        method = method->getTemplateInstantiationPattern();
        if (method)
        {
            body = method->getBody();
            if (!body)
                return;
        }
    }

    BlockScopeRAII scope(m_scopes);
    m_globalVars["$__srcInstance"] = ReflectedObject(m_srcInstance);
    m_globalVars["$__dstInstance"] = ReflectedObject(m_dstInstance);
    m_visibleDecls[method->getParamDecl(0)] = Value::InternalRef(&m_globalVars["$__dstInstance"]);
    m_visibleDecls[method->getParamDecl(1)] = Value::InternalRef(&m_globalVars["$__srcInstance"]);
    ExecuteStatement(body, nullptr);
}

InterpreterImpl::ExecStatementResult InterpreterImpl::ExecuteStatement(const Stmt* stmt, const SwitchCase* curSwithCase)
{
    dbg() << "<><><><><><><><><> Executing statement: " << stmt->getStmtClassName() << std::endl;

    // stmt->

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

    case Stmt::AttributedStmtClass:
        return ExecuteAttributedStatement(cast<AttributedStmt>(stmt), curSwithCase);

    default:
        if (const Expr *expr = dyn_cast<Expr>(stmt))
        {
            ExprScopeRAII scope(m_scopes);
            Value result;
            if (!ExecuteExpression(expr, result))
                return ESR_Failed;
            return ESR_Succeeded;
        }

        std::string stmtClassName = stmt->getStmtClassName();
        stmt->dump();
        dbg() << "<><><><><><><><><> Unknown statement: " << stmtClassName << std::endl;
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

InterpreterImpl::ExecStatementResult InterpreterImpl::ExecuteAttributedStatement(const clang::AttributedStmt* stmt, const SwitchCase* curSwithCase)
{
    auto attrs = GetStatementAttrs(stmt);

    if (attrs.doInject)
    {
        Value::InternalRef foundDecl;
        for (auto& p : m_visibleDecls)
        {
            const clang::NamedDecl* decl = p.first;
            if (decl->getNameAsString() == attrs.targetName)
            {
                foundDecl = p.second;
                break;
            }
        }

        if (foundDecl.pointee == nullptr)
        {
            dbg() << "Can't find declaration for '" << attrs.targetName << "'" << std::endl;
            return ESR_Failed;
        }

        auto reflObject = boost::get<ReflectedObject>(&foundDecl.pointee->GetValue());
        if (!reflObject)
        {
            dbg() << "'" << attrs.targetName << "' is not an reflected object" << std::endl;
            return ESR_Failed;
        }
        auto classPtr = boost::get<ClassInfoPtr>(&reflObject->GetValue());
        if (!classPtr)
        {
            dbg() << "'" << attrs.targetName << "' dosn't denote reflected class type" << std::endl;
            return ESR_Failed;
        }
        DeclInjectionContext context(this, *classPtr);
        m_injectionContextStack.push(&context);
        dbg() << "!!!!!!! '" << attrs.targetName << "' represents class '" << (*classPtr)->name << "'" << std::endl;
        InjectStatement(stmt->getSubStmt(), attrs.targetName, attrs.visibility, true);
        m_injectionContextStack.pop();
        return ESR_Succeeded;
    }

    return ExecuteStatement(stmt->getSubStmt(), curSwithCase);
}


bool InterpreterImpl::ExecuteExpression(const Expr* expr, Value& result)
{
    // expr->dump();
    bool isOk = true;
    ExpressionEvaluator evaluator(this, result, isOk);

    evaluator.Visit(expr);

    return isOk;
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
    dbg() << "Local variable created: " << decl->getNameAsString() << std::endl;

    return &result;
}

bool InterpreterImpl::ExecuteDecl(const Decl* decl)
{
    bool OK = true;

    if (const VarDecl *VD = dyn_cast<VarDecl>(decl))
        OK &= ExecuteVarDecl(VD);

#if 0
    if (const DecompositionDecl *DD = dyn_cast<DecompositionDecl>(D))
        for (auto *BD : DD->bindings())
            if (auto *VD = BD->getHoldingVar())
                OK &= EvaluateDecl(Info, VD);
#endif
    return OK;
}

inline reflection::AccessType ConvertVisibility(const std::string& visibility)
{
    if (visibility == "public")
        return reflection::AccessType::Public;
    else if (visibility == "protected")
        return reflection::AccessType::Protected;
    else if (visibility == "private")
        return reflection::AccessType::Private;

    return reflection::AccessType::Undefined;
}

void InterpreterImpl::InjectMethodDecl(reflection::MethodInfoPtr methodDecl, ClassInfoPtr targetClass, const std::string& visibility)
{
    methodDecl->accessType = ConvertVisibility(visibility);

    targetClass->methods.push_back(std::move(methodDecl));
}

void InterpreterImpl::InjectCodeFragment(const std::string& fragment, ClassInfoPtr targetClass, const std::string& visibility)
{
    reflection::GenericDeclPart declPart;
    declPart.content = fragment;
    declPart.accessType = ConvertVisibility(visibility);

    targetClass->genericParts.push_back(std::move(declPart));
}

struct ObjectInjector : boost::static_visitor<>
{
public:
    ObjectInjector(ClassInfoPtr targetClass, reflection::AccessType accessType)
        : m_target(targetClass)
        , m_accessType(accessType)
    {
    }

    void operator()(MethodInfoPtr method) const
    {
        auto m = std::make_shared<MethodInfo>(*method);
        if (m_accessType != reflection::AccessType::Undefined)
        {
            m->accessType = m_accessType;
        }

        m_target->methods.push_back(m);
    }

    template<typename U>
    void operator()(U) const
    {
    }

private:
    ClassInfoPtr m_target;
    reflection::AccessType m_accessType;
};

struct TypeProjector : boost::static_visitor<reflection::TypeInfo::TypeDescr>
{
public:
    TypeProjector(const clang::ASTContext* context)
        : m_context(context)
    {
    }

    reflection::TypeInfo::TypeDescr operator()(reflection::TypeInfoPtr type) const
    {
        return type->getTypeDescr();
    }

    reflection::TypeInfo::TypeDescr operator()(reflection::ClassInfoPtr type) const
    {
        reflection::TypeInfo::TypeDescr descr;
        reflection::RecordType record;

        record.decl = type->decl;

        descr.isConst = false;
        descr.isReference = false;
        descr.isRVReference = false;
        descr.isVolatile = false;
        descr.name = type->name;
        descr.namespaceQual = type->namespaceQualifier;
        descr.pointingLevels = 0;
        descr.scopeSpec = type->scopeSpecifier;
        descr.type = record;

        return descr;
    }

    template<typename U>
    reflection::TypeInfo::TypeDescr operator()(U) const
    {
        return reflection::TypeInfo::TypeDescr();
    }

private:
    const clang::ASTContext* m_context;
};

void InterpreterImpl::InjectObjectInst(const ReflectedObject& obj, ClassInfoPtr targetClass, const std::string& visibility)
{
    boost::apply_visitor(ObjectInjector(targetClass, ConvertVisibility(visibility)), obj.GetValue());
}

void InterpreterImpl::InjectDeclaration(const clang::Stmt* stmt, const std::string& target, const std::string& visibility)
{
    if (stmt->getStmtClass() == Stmt::CompoundStmtClass)
    {
        const CompoundStmt* s = cast<CompoundStmt>(stmt);
        for (const auto* item : s->body())
            InjectDeclaration(item, target, visibility);
        return;
    }
    else if (stmt->getStmtClass() != Stmt::DeclStmtClass)
    {
        stmt->dump();
        return;
    }

    const clang::DeclStmt* declStmt = dyn_cast_or_null<DeclStmt>(stmt);
    if (declStmt == nullptr)
        return;

    struct DeclAnalyzer : public clang::ConstDeclVisitor<DeclAnalyzer>
    {
        InterpreterImpl* m_interpreter;
        const std::string& m_visibility;

        DeclAnalyzer(InterpreterImpl* interpreter, const std::string& visibility)
            : m_interpreter(interpreter)
            , m_visibility(visibility)
        {}

        void VisitFunctionDecl(const clang::FunctionDecl* decl)
        {
            reflection::AstReflector reflector(m_interpreter->m_astContext, m_interpreter->m_diagReporter->GetConsoleWriter());
            auto methodInfo = reflector.ReflectMethod(decl, nullptr);
            m_interpreter->AdjustMethodParams(methodInfo);

            m_interpreter->m_injectionContextStack.top()->InjectMethodDecl(methodInfo, m_visibility);
        }
    };

    for (const clang::Decl* decl : declStmt->getDeclGroup())
    {
        decl->dump();

        DeclAnalyzer visitor(this, visibility);
        visitor.Visit(decl);
    }
}

void InterpreterImpl::InjectStatement(const clang::Stmt* stmt, const std::string& target, const std::string& visibility, bool isInitial)
{
    switch (stmt->getStmtClass())
    {
    case Stmt::CompoundStmtClass:
    {
        const CompoundStmt* s = cast<CompoundStmt>(stmt);
        for (const auto* item : s->body())
            InjectStatement(item, target, visibility, false);
        return;
    }
    case Stmt::AttributedStmtClass:
    {
        auto attrStmt = cast<AttributedStmt>(stmt);
        auto attrs = GetStatementAttrs(attrStmt);
        if (attrs.isConstexpr)
        {
            ExecuteStatement(attrStmt->getSubStmt(), nullptr);
            return;
        }
        break;
    }
    default:
        break;
    }

    if (isInitial)
    {
        const clang::LambdaExpr* le = dyn_cast_or_null<LambdaExpr>(stmt);
        if (le != nullptr)
        {
            InjectMethod(le, target, visibility);
            return;
        }
        const clang::Expr* e = dyn_cast_or_null<Expr>(stmt);
        if (e != nullptr)
        {
            Value injectedObj;
            if (ExecuteExpression(e, injectedObj))
            {
                InjectObject(injectedObj, target, visibility);
                return;
            }
        }
    }

    // stmt->dump();

    if (m_injectionContextStack.size() == 1 || !target.empty())
    {
        InjectDeclaration(stmt, target, visibility);
    }
    else
    {
        auto content = InjectedCodeRenderer::RenderAsSnippet(this, stmt);

        m_injectionContextStack.top()->InjectCodeFragment(content, visibility);
    }
}

void InterpreterImpl::AdjustMethodParams(reflection::MethodInfoPtr methodDecl)
{
    int paramIdx = 0;
    int tplParamIdx = 0;
    for (auto& p : methodDecl->params)
    {
        auto qType = p.decl->getType();
        auto curIdx = paramIdx ++;
        dbg() << ">>>\tparam: " << p.name << " of type " << p.type << "\n";
        const reflection::TemplateParamType* param = p.type->getAsTemplateParamType();
        const reflection::DecltypeType* decltypeType = p.type->getAsDecltypeType();
        auto descr = p.type->getTypeDescr();
        if (param)
        {
            char tplParamName[4];
            snprintf(tplParamName, sizeof(tplParamName), "T%d", tplParamIdx ++);
            descr.name = tplParamName;

            reflection::TemplateParamInfo tplParamInfo;
            tplParamInfo.tplDeclName = std::string("typename ") + tplParamName;
            tplParamInfo.tplRefName = tplParamName;
            tplParamInfo.kind = reflection::TemplateType::TemplateTplArg;
            methodDecl->tplParams.push_back(std::move(tplParamInfo));
        }
        else if (decltypeType)
        {
            descr = GetProjectedTypeName(decltypeType, descr);
        }
        else
            continue;


        p.type = reflection::TypeInfo::Create(descr);
        p.fullDecl = p.type->getPrintedName() + " " + p.name;
    }

    auto returnType = methodDecl->returnType;
    if (returnType->getAsDecltypeType())
    {
        auto descr = returnType->getTypeDescr();
        methodDecl->returnType = reflection::TypeInfo::Create(GetProjectedTypeName(returnType->getAsDecltypeType(), descr));
    }
}

reflection::TypeInfo::TypeDescr InterpreterImpl::GetProjectedTypeName(const reflection::DecltypeType* type, const reflection::TypeInfo::TypeDescr& origTypeDescr)
{
    type->declTypeExpr->dump();
    Value projectResult;
    if (!ExecuteExpression(type->declTypeExpr, projectResult))
        return reflection::TypeInfo::TypeDescr();

    auto typeDescr = boost::apply_visitor(TypeProjector(m_astContext), boost::get<ReflectedObject>(&GetActualValue(projectResult)->GetValue())->GetValue());

    if (origTypeDescr.isConst)
        typeDescr.isConst = true;

    if (origTypeDescr.isReference)
        typeDescr.isReference = true;

    if (origTypeDescr.isRVReference)
        typeDescr.isRVReference = true;

    typeDescr.pointingLevels += origTypeDescr.pointingLevels;

    return typeDescr;
}

void InterpreterImpl::InjectMethod(const clang::LambdaExpr* le, const std::string& target, const std::string& visibility)
{
    std::cout << ">>> Method injection (via lambda) found" << std::endl;

//    for (auto& cap : le->capture_inits())
//    {
//        std::cout << ">>>\t=======\n";
//        cap->dump();
//    }
    std::string methodName;
    bool isVirtual = false;
    bool isConst = false;
    bool isStatic = false;

    for (const clang::LambdaCapture& cap : le->explicit_captures())
    {
        dbg() << ">>>\tLambdaCapture kind: " << cap.getCaptureKind() << std::endl;
        const clang::VarDecl* varDecl = cap.getCapturedVar();
        if (!varDecl)
            continue;

        std::string varName = varDecl->getName().str();
        dbg() << ">>>\tLambdaCapture name: " << varName << ", isInitializer: " << varDecl->isInitCapture() << std::endl;
        auto* initExpr = varDecl->getInit();
        if (!varDecl->isInitCapture())
        {
//            auto var = CreateLocalVar(varDecl);
//            if (initExpr)
//                ExecuteExpression(initExpr, var->val);
            continue;
        }

        Value initVal;
        if (!ExecuteExpression(initExpr, initVal))
            continue;

        dbg() << ">>>\tInitValueKind: " << initVal.GetValue().which() << std::endl;
        if (varName == "name")
            methodName = boost::get<std::string>(initVal.GetValue());
        else if (varName == "is_const")
            isConst = boost::get<bool>(initVal.GetValue());
        else if (varName == "is_virtual")
            isVirtual = boost::get<bool>(initVal.GetValue());
        else if (varName == "is_static")
            isStatic = boost::get<bool>(initVal.GetValue());
    }

    if (methodName.empty())
        return; // TODO: Handle invalid injection capture

    reflection::AstReflector reflector(m_astContext, m_diagReporter->GetConsoleWriter());

    const CXXMethodDecl* callOperator = le->getCallOperator();
    auto callOperDescr = reflector.ReflectMethod(callOperator, nullptr);

    reflection::MethodInfoPtr methodDecl = std::make_shared<reflection::MethodInfo>();
    methodDecl->name = methodName;
    methodDecl->returnType = callOperDescr->returnType;
    methodDecl->returnTypeAsString = callOperDescr->returnTypeAsString;
    methodDecl->params = callOperDescr->params;
    methodDecl->isConst = isConst;
    methodDecl->isVirtual = isVirtual;
    methodDecl->isStatic = isStatic;

    AdjustMethodParams(methodDecl);

    methodDecl->isInlined = true;
    methodDecl->isClassScopeInlined = true;
    methodDecl->isDefined = true;
    methodDecl->body = InjectedCodeRenderer::RenderAsSnippet(this, le->getBody(), 1);

    m_injectionContextStack.top()->InjectMethodDecl(methodDecl, visibility);
}

void InterpreterImpl::InjectObject(const Value& v, const std::string& target, const std::string& visibility)
{
    dbg() << ">>> Object injection found: " << v.ToString() << std::endl;
    auto reflObj = boost::get<ReflectedObject>(&GetActualValue(v)->GetValue());
    if (!reflObj)
        return;

    m_injectionContextStack.top()->InjectObject(*reflObj, visibility);
}

std::string InterpreterImpl::RenderInjectedConstexpr(const clang::Stmt* stmt)
{
    StatementInjectionContext injectionContext;
    m_injectionContextStack.push(&injectionContext);

    ExecuteStatement(stmt, nullptr);

    m_injectionContextStack.pop();
    return injectionContext.GetRenderResult();
}

nonstd::expected<Value, std::string> InterpreterImpl::GetDeclReference(const clang::NamedDecl* decl)
{
    auto p = m_visibleDecls.find(decl);
    if (p != m_visibleDecls.end())
    {
        dbg() << "Variable resolved: '" + decl->getNameAsString() + "'" << std::endl;
        return Value(p->second);
    }

    for (auto& p : m_visibleDecls)
    {
        if (p.first->getNameAsString() == decl->getNameAsString())
        {
            dbg() << "Variable resolved: '" + decl->getNameAsString() + "'" << std::endl;
            return Value(p.second);
        }
    }

    Value val;
    if (DetectSpecialDecl(decl, val))
        return val;

    return nonstd::make_unexpected("Can't resolve variable '" + decl->getNameAsString() + "'");
}

const clang::VarDecl* FindVariableDecl(const clang::NamedDecl* decl)
{
    auto usd = llvm::dyn_cast_or_null<const clang::UsingShadowDecl>(decl);
    if (usd)
    {
        decl = usd->getTargetDecl();
    }

    const clang::VarDecl* result = llvm::dyn_cast_or_null<const clang::VarDecl>(decl);

    return result;
}

bool InterpreterImpl::DetectSpecialDecl(const clang::NamedDecl* decl, Value& val)
{
    const clang::VarDecl* varDecl = FindVariableDecl(decl);

    if (!varDecl)
        return false;

    reflection::TypeInfoPtr ti = reflection::TypeInfo::Create(varDecl->getType(), m_astContext);
    std::string varName = varDecl->getNameAsString();
    std::string valueKey = varName + ":>" + ti->getFullQualifiedName();
    Value* actual = &m_globalVars[valueKey];
    if (valueKey == "compiler:>meta::CompilerImpl")
    {
        *actual = ReflectedObject(Compiler());
    }

    /*
    else if(varName.length() >= 1 && varName[0] == '$')
    {
        std::string tmpName = "MetaClass_" + varName.substr(1);
        dbg() << "Possible reference to metaclass found: " << tmpName << std::endl;
        const DeclContext* ctx = decl->getDeclContext();
        if (ctx->isRecord())
        {
            auto rec = llvm::dyn_cast_or_null<const clang::RecordDecl>(ctx);
            // assert(rec);
            std::string name = rec->getNameAsString();
            if (name == tmpName)
            {
                auto tmp = m_instance;
                *actual = ReflectedObject(tmp);
            }
        }
    }*/

    if (!actual->IsEmpty())
    {
        val = Value::InternalRef(actual);
        m_visibleDecls[decl] = Value::InternalRef(actual);
    }
    else
        return false;

    dbg() << "Found reference to variable declaration. Variable name: " << varDecl->getNameAsString() << "', variable type: '" << ti->getFullQualifiedName() << "'" << std::endl;
    return true;
}

bool InterpreterImpl::Report(MessageType type, const clang::SourceLocation& loc, std::string message)
{

    return !(type == MessageType::Error);
}

PrintingPolicy InterpreterImpl::GetDefaultPrintingPolicy()
{
    PrintingPolicy pp(m_astContext->getLangOpts());
    pp.Bool = true;
    return pp;
}
} // interpreter

CppInterpreter::CppInterpreter(const clang::ASTContext* astContext, IDiagnosticReporter* diagReporter)
    : m_astContext(astContext)
    , m_diagReporter(diagReporter)
{

}

CppInterpreter::~CppInterpreter()
{
}

void CppInterpreter::Execute(reflection::MethodInfoPtr generator, reflection::ClassInfoPtr dst, reflection::ClassInfoPtr src)
{
    interpreter::InterpreterImpl i(m_astContext, m_diagReporter, dst, src);
    i.ExecuteMetaclassMethod(generator->decl);
}

} // codegen

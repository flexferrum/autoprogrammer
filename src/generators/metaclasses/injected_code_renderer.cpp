#include "injected_code_renderer.h"
#include "cpp_interpreter_impl.h"
#include "value_ops.h"

#include <llvm/Support/raw_ostream.h>

namespace codegen
{
namespace interpreter
{

bool InjectedCodeRenderer::TraverseAttributedStmt(clang::AttributedStmt* stmt)
{
    std::cout << ">>> InjectedCodeRenderer::VisitAttributedStmt <<<" << std::endl;
    auto attrs = GetStatementAttrs(stmt);
    if (attrs.isConstexpr)
    {
        auto innerStmt = stmt->getSubStmt();
        std::string result = m_interpreter->RenderInjectedConstexpr(innerStmt);
        // ReplaceStatement(stmt, "");
        ReplaceStatement(stmt, result);
        return true;
    }

    return BaseClass::TraverseAttributedStmt(stmt);
}

bool InjectedCodeRenderer::VisitCXXMemberCallExpr(clang::CXXMemberCallExpr* expr)
{
    const clang::CXXRecordDecl* rec = expr->getRecordDecl();
    const clang::CXXMethodDecl* method = expr->getMethodDecl();

    std::string recName = rec->getQualifiedNameAsString();
    std::string fnName = method->getNameAsString();
    std::cout << ">>> InjectedCodeRenderer::VisitCXXMemberCallExpr for method '" << recName + "::" + method->getNameAsString() << "'" << std::endl;
    if (recName == "meta::detail::Projector" && fnName == "$_project_member")
    {
        auto callExpr = expr->getCallee();
        const clang::MemberExpr* memExpr = clang::dyn_cast_or_null<clang::MemberExpr>(callExpr);
        Value member;
        if (m_interpreter->ExecuteExpression(expr->getArg(0), member))
        {
            ReplaceRange(memExpr->getMemberLoc(), memExpr->getEndLoc(), member.ToString());
        }
    }
    // dbg() << "[ExpressionEvaluator] Call method '" << method->getNameAsString() << "' from '" << recName << "'" << std::endl;
    return BaseClass::VisitCXXMemberCallExpr(expr);
}

bool InjectedCodeRenderer::VisitCallExpr(clang::CallExpr* expr)
{
    const clang::FunctionDecl* callee = expr->getDirectCallee();
    std::cout << ">>> InjectedCodeRenderer::VisitCallExpr for function '" << (!callee ? std::string() : callee->getQualifiedNameAsString()) << "'" << std::endl;

    if (!callee)
        return true;

    std::string funcName = callee->getQualifiedNameAsString();
	if (funcName == "meta::project")
	{
        Value result;
        m_interpreter->ExecuteExpression(expr->getArg(0), result);

        ReplaceStatement(expr, result.ToString());
    }

    return true;
}

bool InjectedCodeRenderer::VisitCXXStaticCastExpr(clang::CXXStaticCastExpr* stmt)
{
    // stmt->dump();
    auto typeInfo = reflection::TypeInfo::Create(stmt->getType(), m_interpreter->m_astContext); // .dump();
    auto decltypeType = typeInfo->getAsDecltypeType();
    if (decltypeType != nullptr)
    {
        auto newDescr = m_interpreter->GetProjectedTypeName(decltypeType, typeInfo->getTypeDescr());
        auto newTypeInfo = reflection::TypeInfo::Create(newDescr);
        auto range = stmt->getAngleBrackets();
        ReplaceRange(range.getBegin(), range.getEnd(), "<" + newTypeInfo->getPrintedName() + ">");
    }
    return BaseClass::VisitCXXStaticCastExpr(stmt);
}

bool InjectedCodeRenderer::VisitDeclaratorDecl(clang::DeclaratorDecl * stmt)
{
    auto type = stmt->getTypeSourceInfo();
    auto typeInfo = reflection::TypeInfo::Create(stmt->getType(), m_interpreter->m_astContext);
    auto decltypeType = typeInfo->getAsDecltypeType();
    if (decltypeType != nullptr)
    {
        auto newDescr = m_interpreter->GetProjectedTypeName(decltypeType, typeInfo->getTypeDescr());
        auto newTypeInfo = reflection::TypeInfo::Create(newDescr);
        auto range = type->getTypeLoc().getSourceRange();
        ReplaceRange(range.getBegin(), range.getEnd(), newTypeInfo->getPrintedName());
    }
    return BaseClass::VisitDeclaratorDecl(stmt);
}

std::string InjectedCodeRenderer::RenderAsSnippet(InterpreterImpl* i, const clang::Stmt* stmt, unsigned buffShift)
{
    std::cout << "####### InjectedCodeRenderer::RenderAsSnippet Invoked" << std::endl;
    auto& srcMgr = i->m_astContext->getSourceManager();
    auto locStart = stmt->getBeginLoc();
    auto locEnd = stmt->getEndLoc();
    unsigned startOffset = 0;
    unsigned endOffset = 0;
    GetOffsets(i, locStart, locEnd, startOffset, endOffset);
    startOffset += buffShift;
    endOffset -= buffShift;
    auto len = endOffset - startOffset;

    auto buff = srcMgr.getCharacterData(locStart);
    auto buffEnd = buff + len;

    if (buffEnd[0] == ';')
        buffEnd ++;

    std::string content;
    if (buffEnd - buff > buffShift)
        content = std::string(buff + buffShift, buffEnd);

    InjectedCodeRenderer visitor(i, std::move(content), startOffset);
    stmt->dump();
    // visitor.VisitStmt(const_cast<clang::Stmt*>(stmt));
    visitor.TraverseStmt(const_cast<clang::Stmt*>(stmt));

    auto result = visitor.GetRenderResult();;

    return result;
}

std::string InjectedCodeRenderer::GetRenderResult()
{
    std::string result;
    {
        llvm::raw_string_ostream os(result);

        m_rewriteBuffer.write(os);
    }

    return result;
}

void InjectedCodeRenderer::ReplaceRange(clang::SourceLocation locStart, clang::SourceLocation locEnd, const std::string& text)
{
    unsigned startOffset = 0;
    unsigned endOffset = 0;
    GetOffsets(m_interpreter, locStart, locEnd, startOffset, endOffset);
    auto len = endOffset - startOffset;

    auto origText = m_origBuffer.substr(startOffset - m_baseOffset, len);

    std::cout << "Replace text '" << origText << "' with '" << text << "'" << std::endl;

    m_rewriteBuffer.ReplaceText(startOffset - m_baseOffset, len, text);
}

void InjectedCodeRenderer::ReplaceStatement(const clang::Stmt* stmt, const std::string& text)
{
    auto locStart = stmt->getBeginLoc();
    auto locEnd = stmt->getEndLoc();
    ReplaceRange(locStart, locEnd, text);
}

void InjectedCodeRenderer::GetOffsets(InterpreterImpl* i, clang::SourceLocation& start, clang::SourceLocation& end, unsigned& startOff, unsigned& endOff)
{
    auto& srcMgr = i->m_astContext->getSourceManager();

    if (srcMgr.isMacroBodyExpansion(start))
        start = srcMgr.getExpansionLoc(start);
    if (srcMgr.isMacroBodyExpansion(end))
        end = srcMgr.getExpansionRange(end).getAsRange().getEnd();

    startOff = srcMgr.getFileOffset(start);
    endOff = srcMgr.getFileOffset(end) + 1;
}

}
}

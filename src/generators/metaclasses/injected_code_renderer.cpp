#include "injected_code_renderer.h"
#include "cpp_interpreter_impl.h"

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

bool InjectedCodeRenderer::VisitCallExpr(clang::CallExpr* expr)
{
    const clang::FunctionDecl* callee = expr->getDirectCallee();
    std::cout << ">>> InjectedCodeRenderer::VisitCallExpr for function '" << (!callee ? std::string() : callee->getQualifiedNameAsString()) << "'" << std::endl;

    if (!callee)
        return true;

    std::string funcName = callee->getQualifiedNameAsString();
    if (funcName != "meta::project")
        return true;

    Value result;
    m_interpreter->ExecuteExpression(expr->getArg(0), result);

    ReplaceStatement(expr, result.ToString());

    return true;
}

std::string InjectedCodeRenderer::RenderAsSnippet(InterpreterImpl* i, const clang::Stmt* stmt, unsigned buffShift)
{
    std::cout << "####### InjectedCodeRenderer::RenderAsSnippet Invoked" << std::endl;
    auto& srcMgr = i->m_astContext->getSourceManager();
    auto locStart = stmt->getLocStart();
    auto locEnd = stmt->getLocEnd();
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

    std::string content(buff + buffShift, buffEnd);

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

void InjectedCodeRenderer::ReplaceStatement(const clang::Stmt* stmt, const std::string& text)
{
    auto locStart = stmt->getLocStart();
    auto locEnd = stmt->getLocEnd();
    unsigned startOffset = 0;
    unsigned endOffset = 0;
    GetOffsets(m_interpreter, locStart, locEnd, startOffset, endOffset);
    auto len = endOffset - startOffset;

    auto origText = m_origBuffer.substr(startOffset - m_baseOffset, len);

    std::cout << "Replace text '" << origText << "' with '" << text << "'" << std::endl;

    m_rewriteBuffer.ReplaceText(startOffset - m_baseOffset, len, text);
}

void InjectedCodeRenderer::GetOffsets(InterpreterImpl* i, clang::SourceLocation& start, clang::SourceLocation& end, unsigned& startOff, unsigned& endOff)
{
    auto& srcMgr = i->m_astContext->getSourceManager();

    if (srcMgr.isMacroBodyExpansion(start))
        start = srcMgr.getExpansionLoc(start);
    if (srcMgr.isMacroBodyExpansion(end))
        end = srcMgr.getExpansionRange(end).second;

    startOff = srcMgr.getFileOffset(start);
    endOff = srcMgr.getFileOffset(end) + 1;
}

}
}

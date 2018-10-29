#include "injected_code_renderer.h"
#include "cpp_interpreter_impl.h"

#include <llvm/Support/raw_ostream.h>

namespace codegen
{
namespace interpreter
{
    
bool InjectedCodeRenderer::VisitAttributedStmt(clang::AttributedStmt* stmt)
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
    
    return true;
}
    
std::string InjectedCodeRenderer::RenderAsSnippet(InterpreterImpl* i, const clang::Stmt* stmt, unsigned buffShift)
{
    std::cout << "####### InjectedCodeRenderer::RenderAsSnippet Invoked" << std::endl;
    auto& srcMgr = i->m_astContext->getSourceManager();
    auto locStart = stmt->getLocStart();
    auto locEnd = stmt->getLocEnd();
    auto startOffset = srcMgr.getFileOffset(locStart);
    auto len = srcMgr.getFileOffset(locEnd) - startOffset;

    auto buff = srcMgr.getCharacterData(locStart);
    std::string content(buff + buffShift, buff + len);
    
    InjectedCodeRenderer visitor(i, std::move(content), startOffset + buffShift);
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
    auto& srcMgr = m_interpreter->m_astContext->getSourceManager();
    auto locStart = stmt->getLocStart();
    if (srcMgr.isMacroBodyExpansion(locStart))
        locStart = srcMgr.getExpansionLoc(locStart);
    auto locEnd = stmt->getLocEnd();
    if (srcMgr.isMacroBodyExpansion(locEnd))
        locEnd = srcMgr.getExpansionRange(locEnd).second;
    auto startOffset = srcMgr.getFileOffset(locStart);
    auto endOffset = srcMgr.getFileOffset(locEnd) + 1;
    auto len = endOffset - startOffset;
    
    bool isMacroExpStart = srcMgr.isMacroBodyExpansion(locStart);
    bool isMacroExpEnd = srcMgr.isMacroBodyExpansion(locEnd);
    
    std::cout << "Replace text between (" << startOffset << ", " << endOffset << ", " << isMacroExpStart << ", " << isMacroExpEnd << ") with " << text << std::endl;
    
    m_rewriteBuffer.ReplaceText(startOffset - m_baseOffset, len, text);
}

}
}
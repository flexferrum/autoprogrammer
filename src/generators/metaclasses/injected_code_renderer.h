#ifndef INJECTED_CODE_RENDERER_IMPL_H
#define INJECTED_CODE_RENDERER_IMPL_H

#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Rewrite/Core/RewriteBuffer.h>

namespace codegen
{
namespace interpreter
{
 
 
class InterpreterImpl;
class InjectedCodeRenderer : public clang::RecursiveASTVisitor<InjectedCodeRenderer>
{
public:
    using BaseClass = clang::RecursiveASTVisitor<InjectedCodeRenderer>;
    
    InjectedCodeRenderer(InterpreterImpl* i, std::string origText, unsigned baseOffset)
        : m_interpreter(i)
        , m_baseOffset(baseOffset)
        , m_origBuffer(std::move(origText))
    {
        m_rewriteBuffer.Initialize(m_origBuffer.data(), m_origBuffer.data() + m_origBuffer.size());
    }
    
    static std::string RenderAsSnippet(InterpreterImpl* i, const clang::Stmt* stmt, unsigned buffShift = 0);
    
    std::string GetRenderResult();
    
    bool VisitAttributedStmt(clang::AttributedStmt* stmt);
    
private:
    void ReplaceStatement(const clang::Stmt* stmt, const std::string& text);

private:
    InterpreterImpl* m_interpreter;
    unsigned m_baseOffset;    
    std::string m_origBuffer;
    clang::RewriteBuffer m_rewriteBuffer;
};
}
}

#endif // INJECTED_CODE_RENDERER_IMPL_H
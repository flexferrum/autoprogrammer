#ifndef AST_UTILS_H
#define AST_UTILS_H

#include "decls_reflection.h"
#include "cpp_source_stream.h"

#include <clang/Basic/SourceManager.h>
#include <clang/AST/PrettyPrinter.h>
#include <clang/AST/ASTContext.h>

namespace reflection 
{

inline auto GetLocation(const clang::SourceLocation& loc, const clang::ASTContext* context)
{
    SourceLocation result;
    clang::PresumedLoc ploc = context->getSourceManager().getPresumedLoc(loc, false);
    result.fileName = ploc.getFilename();
    result.line = ploc.getLine();
    result.column = ploc.getColumn();
    
    return result;
}

template<typename Entity>
auto GetLocation(const Entity* decl, const clang::ASTContext* context)
{
    return GetLocation(decl->getLocation(), context); 
}

inline void SetupDefaultPrintingPolicy(clang::PrintingPolicy& policy)
{
    policy.Bool = true;
    policy.AnonymousTagLocations = false;
    policy.SuppressUnwrittenScope = true;
    policy.Indentation = 4;
    policy.UseVoidForZeroParams = false;
    policy.SuppressInitializers = true;
}

template<typename Entity>
std::string EntityToString(const Entity* decl, const clang::ASTContext* context)
{
    clang::PrintingPolicy policy(context->getLangOpts());
    
    SetupDefaultPrintingPolicy(policy);
    
    std::string result;
    {
        llvm::raw_string_ostream os(result);
    
        decl->print(os, policy);
    }
    return result;
}


template<typename Fn>
void WriteNamespaceContents(codegen::CppSourceStream &hdrOs, reflection::NamespaceInfoPtr ns, Fn&& fn)
{
    using namespace codegen;
    
    out::BracedStreamScope nsScope("namespace " + ns->name, "", 0);
    if (!ns->isRootNamespace)
        hdrOs << out::new_line << nsScope;
    
    fn(hdrOs, ns);
    for (auto& inner : ns->innerNamespaces)
        WriteNamespaceContents(hdrOs, inner, std::forward<Fn>(fn));
}

} // reflection

#endif // AST_UTILS_H

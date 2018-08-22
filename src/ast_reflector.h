#ifndef AST_REFLECTOR_H
#define AST_REFLECTOR_H

#include "decls_reflection.h"

namespace reflection
{

class AstReflector
{
public:
    explicit AstReflector(const clang::ASTContext* context)
        : m_astContext(context)
    {
    }

    EnumInfoPtr ReflectEnum(const clang::EnumDecl* decl, NamespacesTree* nsTree);
    TypedefInfoPtr ReflectTypedef(const clang::TypedefNameDecl* decl, NamespacesTree* nsTree);
    ClassInfoPtr ReflectClass(const clang::CXXRecordDecl* decl, NamespacesTree* nsTree);
    MethodInfoPtr ReflectMethod(const clang::CXXMethodDecl* decl, NamespacesTree* nsTree);

    static void SetupNamedDeclInfo(const clang::NamedDecl* decl, NamedDeclInfo* info, const clang::ASTContext* astContext);

private:
    const clang::NamedDecl* FindEnclosingOpaqueDecl(const clang::DeclContext* decl);
    void ReflectImplicitSpecialMembers(const clang::CXXRecordDecl* decl, ClassInfo* classInfo, NamespacesTree* nsTree);

private:
    const clang::ASTContext* m_astContext;
};

} // reflection

#endif // AST_REFLECTOR_H

#include "ast_reflector.h"
#include "ast_utils.h"
#include "type_info.h"

#include <clang/AST/ASTContext.h>
#include <boost/algorithm/string/replace.hpp>

#include <iostream>

namespace reflection
{

using namespace clang;

template<typename Cont>
auto FindExisting(const Cont& cont, const std::string& qualifiedName)
{
    auto p = std::find_if(begin(cont), end(cont),
         [&qualifiedName](auto& item) {return item->decl->getQualifiedNameAsString() == qualifiedName;});


    return p == end(cont) ? Cont::value_type() : *p;
}

AccessType ConvertAccessType(clang::AccessSpecifier access)
{
    AccessType result = AccessType::Undefined;
    switch (access)
    {
    case clang::AS_public:
        result = AccessType::Public;
        break;
    case clang::AS_protected:
        result = AccessType::Protected;
        break;
    case clang::AS_private:
        result = AccessType::Private;
        break;
    case clang::AS_none:
        break;

    }

    return result;
}

EnumInfoPtr AstReflector::ReflectEnum(const clang::EnumDecl *decl, NamespacesTree* nsTree)
{
    const DeclContext* nsContext = decl->getEnclosingNamespaceContext();

    NamespaceInfoPtr ns;
    EnumInfoPtr enumInfo;
    if (nsTree != nullptr)
    {
        ns = nsTree->GetNamespace(nsContext);
        enumInfo = FindExisting(ns->enums, decl->getQualifiedNameAsString());
    }

    if (enumInfo)
        return enumInfo;

    // const NamedDecl* parentDecl = FindEnclosingOpaqueDecl(decl);

    enumInfo = std::make_shared<EnumInfo>();
    enumInfo->decl = decl;
    enumInfo->location = GetLocation(decl, m_astContext);

    SetupNamedDeclInfo(decl, enumInfo.get(), m_astContext);

    enumInfo->isScoped = decl->isScoped();

    for (const EnumConstantDecl* itemDecl : decl->enumerators())
    {
        EnumItemInfo item;
        item.itemName = itemDecl->getNameAsString();
        item.itemValue = itemDecl->getInitVal().toString(10);
        item.location = GetLocation(itemDecl, m_astContext);
        enumInfo->items.push_back(std::move(item));
    }

    if (ns)
        ns->enums.push_back(enumInfo);

    return enumInfo;
}

ClassInfoPtr AstReflector::ReflectClass(const CXXRecordDecl* decl, NamespacesTree* nsTree)
{
    const DeclContext* nsContext = decl->getEnclosingNamespaceContext();

    NamespaceInfoPtr ns;
    ClassInfoPtr classInfo;

    if (nsTree)
    {
        ns = nsTree->GetNamespace(nsContext);
        classInfo = FindExisting(ns->classes, decl->getQualifiedNameAsString());
    }

    if (classInfo)
        return classInfo;

    classInfo = std::make_shared<ClassInfo>();
    classInfo->decl = decl;

    SetupNamedDeclInfo(decl, classInfo.get(), m_astContext);
    classInfo->isUnion = decl->isUnion();
    classInfo->location = GetLocation(decl, m_astContext);

    ReflectImplicitSpecialMembers(decl, classInfo.get(), nsTree);

    for (auto methodDecl : decl->methods())
    {
        MethodInfoPtr methodInfo = ReflectMethod(methodDecl, nsTree);
        classInfo->methods.push_back(methodInfo);
    }

    for (auto memberDecl : decl->fields())
    {
        if (memberDecl->isAnonymousStructOrUnion())
            continue;

        auto memberInfo = std::make_shared<MemberInfo>();
        SetupNamedDeclInfo(memberDecl, memberInfo.get(), m_astContext);
        memberInfo->type = TypeInfo::Create(memberDecl->getType(), m_astContext);
        // memberInfo->isStatic = memberDecl->isStatic();
        memberInfo->accessType = ConvertAccessType(memberDecl->getAccess());
        classInfo->members.push_back(memberInfo);
    }

    if (decl->hasDefinition())
    {
        classInfo->isAbstract = decl->isAbstract();
        classInfo->isTrivial = decl->isTrivial();
        classInfo->hasDefinition = true;
        for (auto& base : decl->bases())
        {
            ;
        }
    }

    if (ns)
        ns->classes.push_back(classInfo);

    return classInfo;
}

void AstReflector::ReflectImplicitSpecialMembers(const CXXRecordDecl* decl, ClassInfo* classInfo, NamespacesTree* nsTree)
{
    auto setupImplicitMember = [this, decl, classInfo](const std::string& name)
    {
        MethodInfoPtr methodInfo = std::make_shared<MethodInfo>();
        methodInfo->scopeSpecifier = classInfo->GetFullQualifiedName(false);
        methodInfo->namespaceQualifier = classInfo->namespaceQualifier;
        methodInfo->name = name;
        methodInfo->accessType = AccessType::Public;
        methodInfo->declLocation = classInfo->location;
        methodInfo->defLocation = classInfo->location;
        methodInfo->isImplicit = true;

        return methodInfo;
    };

    auto setupConstructor = [this, decl, classInfo, &setupImplicitMember]()
    {
        MethodInfoPtr methodInfo = setupImplicitMember(classInfo->name);
        methodInfo->isCtor = true;

        return methodInfo;
    };

    if (!decl->hasUserProvidedDefaultConstructor() && decl->hasDefaultConstructor())
    {
        auto ctorInfo = setupConstructor();
        ctorInfo->constructorType = ConstructorType::Default;
        classInfo->methods.push_back(ctorInfo);
    }

    if (!decl->hasUserDeclaredCopyConstructor() && decl->hasCopyConstructorWithConstParam())
    {
        auto rt = m_astContext->getRecordType(decl);
        auto constRefT = m_astContext->getLValueReferenceType(m_astContext->getConstType(rt));
        auto ctorInfo = setupConstructor();
        ctorInfo->constructorType = ConstructorType::Copy;

        MethodParamInfo paramInfo;
        paramInfo.name = "val";
        paramInfo.type = TypeInfo::Create(constRefT, m_astContext);
        paramInfo.fullDecl = EntityToString(&constRefT, m_astContext) + " val";
        ctorInfo->params.push_back(std::move(paramInfo));

        classInfo->methods.push_back(ctorInfo);
    }

    if (!decl->hasUserDeclaredMoveConstructor() && decl->hasMoveConstructor())
    {
        auto rt = m_astContext->getRecordType(decl);
        auto constRefT = m_astContext->getRValueReferenceType(m_astContext->getConstType(rt));
        auto ctorInfo = setupConstructor();
        ctorInfo->constructorType = ConstructorType::Move;

        MethodParamInfo paramInfo;
        paramInfo.name = "val";
        paramInfo.type = TypeInfo::Create(constRefT, m_astContext);
        paramInfo.fullDecl = EntityToString(&constRefT, m_astContext) + " val";
        ctorInfo->params.push_back(std::move(paramInfo));

        classInfo->methods.push_back(ctorInfo);
    }

    if (!decl->hasUserDeclaredDestructor() && decl->hasSimpleDestructor())
    {
        auto dtorInfo = setupImplicitMember("~" + classInfo->name);
        dtorInfo->isDtor = true;
        classInfo->methods.push_back(dtorInfo);
    }

    if (!decl->hasUserDeclaredCopyAssignment() && decl->hasCopyAssignmentWithConstParam())
    {
        auto rt = m_astContext->getRecordType(decl);
        auto constRefT = m_astContext->getLValueReferenceType(m_astContext->getConstType(rt));
        auto operInfo = setupImplicitMember("operator=");
        operInfo->isOperator = true;
        operInfo->assignmentOperType = AssignmentOperType::Copy;

        MethodParamInfo paramInfo;
        paramInfo.name = "val";
        paramInfo.type = TypeInfo::Create(constRefT, m_astContext);
        paramInfo.fullDecl = EntityToString(&constRefT, m_astContext) + " val";
        operInfo->params.push_back(std::move(paramInfo));

        classInfo->methods.push_back(operInfo);
    }

    if (!decl->hasUserDeclaredMoveConstructor() && decl->hasMoveAssignment())
    {
        auto rt = m_astContext->getRecordType(decl);
        auto constRefT = m_astContext->getRValueReferenceType(m_astContext->getConstType(rt));
        auto operInfo = setupImplicitMember("operator=");
        operInfo->isOperator = true;
        operInfo->assignmentOperType = AssignmentOperType::Move;

        MethodParamInfo paramInfo;
        paramInfo.name = "val";
        paramInfo.type = TypeInfo::Create(constRefT, m_astContext);
        paramInfo.fullDecl = EntityToString(&constRefT, m_astContext) + " val";
        operInfo->params.push_back(std::move(paramInfo));

        classInfo->methods.push_back(operInfo);
    }
}

MethodInfoPtr AstReflector::ReflectMethod(const CXXMethodDecl* decl, NamespacesTree* nsTree)
{
    MethodInfoPtr methodInfo = std::make_shared<MethodInfo>();

    const DeclContext* nsContext = decl->getEnclosingNamespaceContext();

    NamespaceInfoPtr ns;
    if (nsTree)
    {
        ns = nsTree->GetNamespace(nsContext);
        SetupNamedDeclInfo(decl, methodInfo.get(), m_astContext);
    }

    QualType fnQualType = decl->getType();

    while (const ParenType *parenType = llvm::dyn_cast<ParenType>(fnQualType))
        fnQualType = parenType->getInnerType();

    const FunctionProtoType* fnProtoType = nullptr;
    if (const FunctionType *fnType = fnQualType->getAs<FunctionType>())
    {
        if (decl->hasWrittenPrototype())
            fnProtoType = llvm::dyn_cast<FunctionProtoType>(fnType);
    }

    auto ctor = llvm::dyn_cast_or_null<const clang::CXXConstructorDecl>(decl);

    methodInfo->isCtor = ctor != nullptr;
    if (ctor != nullptr)
    {
        if (ctor->isDefaultConstructor())
            methodInfo->constructorType = ConstructorType::Default;
        else if (ctor->isCopyConstructor())
            methodInfo->constructorType = ConstructorType::Copy;
        else if (ctor->isMoveConstructor())
            methodInfo->constructorType = ConstructorType::Move;
        else if (ctor->isConvertingConstructor(true))
            methodInfo->constructorType = ConstructorType::Convert;
        methodInfo->isExplicitCtor = ctor->isExplicit();
    }
    methodInfo->isDtor = llvm::dyn_cast_or_null<const clang::CXXDestructorDecl>(decl) != nullptr;
    methodInfo->isOperator = decl->isOverloadedOperator();
    methodInfo->isDeleted = decl->isDeleted();
    methodInfo->isConst = decl->isConst();
    methodInfo->isImplicit = decl->isImplicit();
    if (fnProtoType)
        methodInfo->isNoExcept = isNoexceptExceptionSpec(fnProtoType->getExceptionSpecType());
    // methodInfo->isNoExcept = decl->is
    methodInfo->isPure = decl->isPure();
    methodInfo->isStatic = decl->isStatic();
    methodInfo->isVirtual = decl->isVirtual();
    methodInfo->accessType = ConvertAccessType(decl->getAccess());
    methodInfo->fullPrototype = EntityToString(decl, m_astContext);
    methodInfo->decl = decl;
    methodInfo->returnType = TypeInfo::Create(decl->getReturnType(), m_astContext);

    methodInfo->declLocation = GetLocation(decl, m_astContext);
    auto defDecl = decl->getDefinition();
    if (defDecl != nullptr)
        methodInfo->defLocation = GetLocation(defDecl, m_astContext);

    for (const clang::ParmVarDecl* param: decl->parameters())
    {
        MethodParamInfo paramInfo;
        paramInfo.name = param->getNameAsString();
        paramInfo.type = TypeInfo::Create(param->getType(), m_astContext);
        paramInfo.fullDecl = EntityToString(param, m_astContext);
        methodInfo->params.push_back(std::move(paramInfo));
    }

    return methodInfo;
}

void AstReflector::SetupNamedDeclInfo(const NamedDecl* decl, NamedDeclInfo* info, const clang::ASTContext* astContext)
{
    info->name = decl->getNameAsString();

    auto declContext = decl->getDeclContext();
    const clang::NamespaceDecl* encNs = nullptr; // = clang::NamespaceDecl::castFromDeclContext(declContext->isNamespace() ? declContext : declContext->getEnclosingNamespaceContext());
    // auto encNsCtx = clang::NamespaceDecl::castToDeclContext(encNs);

    std::string scopeQualifier = "";

    for (const DeclContext* parentCtx = declContext; parentCtx != nullptr; parentCtx = parentCtx->getParent())
    {
        const NamedDecl* namedScope = llvm::dyn_cast_or_null<const NamedDecl>(Decl::castFromDeclContext(parentCtx));
        if (namedScope == nullptr)
            continue;

        if (parentCtx->isNamespace())
        {
            encNs = clang::NamespaceDecl::castFromDeclContext(parentCtx);
            break;
        }

        auto scopeName = namedScope->getNameAsString();
        if (scopeName.empty())
            continue;

        if (scopeQualifier.empty())
            scopeQualifier = scopeName;
        else
            scopeQualifier = scopeName + "::" + scopeQualifier;
    }

    info->scopeSpecifier = scopeQualifier;

    if (encNs != nullptr && !encNs->isTranslationUnit())
    {
        clang::PrintingPolicy policy(astContext->getLangOpts());
        SetupDefaultPrintingPolicy(policy);

        llvm::raw_string_ostream os(info->namespaceQualifier);
        encNs->printQualifiedName(os, policy);
    }

    boost::algorithm::replace_all(info->namespaceQualifier, "(anonymous)::", "");
    boost::algorithm::replace_all(info->namespaceQualifier, "::(anonymous)", "");
    boost::algorithm::replace_all(info->namespaceQualifier, "(anonymous)", "");
}


} // reflection

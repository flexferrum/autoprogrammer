#include "type_info.h"
#include "ast_utils.h"
#include "ast_reflector.h"
#include "utils.h"

#include <clang/AST/TypeVisitor.h>
#include <clang/AST/DeclTemplate.h>

#include <boost/algorithm/string/find.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <iostream>
#include <sstream>

namespace reflection
{

class TypeUnwrapper : public clang::TypeVisitor<TypeUnwrapper, bool>
{
public:
    TypeUnwrapper(TypeInfo* targetType, const clang::ASTContext* astContext)
        : m_targetType(targetType)
        , m_astContext(astContext)
    {
    }

    bool VisitBuiltinType(const clang::BuiltinType* tp)
    {
        BuiltinType result;
        switch (tp->getKind())
        {
        case clang::BuiltinType::Void:
            result.type = BuiltinType::Void;
            m_targetType->m_declaredName = "void";
            break;
        case clang::BuiltinType::Bool:
            result.type = BuiltinType::Bool;
            result.bits = 1;
            m_targetType->m_declaredName = "bool";
            break;
        case clang::BuiltinType::Char_U:
        case clang::BuiltinType::Char_S:
            result.type = BuiltinType::Char;
            result.bits = 8;
            m_targetType->m_declaredName = "char";
            break;
        case clang::BuiltinType::WChar_U:
        case clang::BuiltinType::WChar_S:
            result.type = BuiltinType::Char;
            result.bits = 32;
            m_targetType->m_declaredName = "wchar_t";
            break;
        case clang::BuiltinType::SChar:
            result.type = BuiltinType::Char;
            result.bits = 8;
            result.isSigned = BuiltinType::Signed;
            m_targetType->m_declaredName = "signed char";
            break;
        case clang::BuiltinType::UChar:
            result.type = BuiltinType::Char;
            result.bits = 8;
            result.isSigned = BuiltinType::Unsigned;
            m_targetType->m_declaredName = "unsigned char";
            break;
        case clang::BuiltinType::Char16:
            result.type = BuiltinType::Char;
            result.bits = 16;
            m_targetType->m_declaredName = "char16_t";
            break;
        case clang::BuiltinType::Char32:
            result.type = BuiltinType::Char;
            result.bits = 32;
            m_targetType->m_declaredName = "char32_t";
            break;
        case clang::BuiltinType::UShort:
            result.type = BuiltinType::Short;
            result.bits = 16;
            result.isSigned = BuiltinType::Unsigned;
            m_targetType->m_declaredName = "unsigned short";
            break;
        case clang::BuiltinType::Short:
            result.type = BuiltinType::Short;
            result.bits = 16;
            result.isSigned = BuiltinType::Signed;
            m_targetType->m_declaredName = "short";
            break;
        case clang::BuiltinType::UInt:
            result.type = BuiltinType::Int;
            result.bits = 32;
            result.isSigned = BuiltinType::Unsigned;
            m_targetType->m_declaredName = "unsigned int";
            break;
        case clang::BuiltinType::Int:
            result.type = BuiltinType::Int;
            result.bits = 32;
            result.isSigned = BuiltinType::Signed;
            m_targetType->m_declaredName = "int";
            break;
        case clang::BuiltinType::ULong:
            result.type = BuiltinType::Long;
            result.bits = 64;
            result.isSigned = BuiltinType::Unsigned;
            m_targetType->m_declaredName = "unsigned long";
            break;
        case clang::BuiltinType::Long:
            result.type = BuiltinType::Long;
            result.bits = 64;
            result.isSigned = BuiltinType::Signed;
            m_targetType->m_declaredName = "long";
            break;
        case clang::BuiltinType::ULongLong:
            result.type = BuiltinType::LongLong;
            result.bits = 64;
            result.isSigned = BuiltinType::Unsigned;
            m_targetType->m_declaredName = "unsigned long long";
            break;
        case clang::BuiltinType::LongLong:
            result.type = BuiltinType::LongLong;
            result.bits = 64;
            result.isSigned = BuiltinType::Signed;
            m_targetType->m_declaredName = "long long";
            break;
        case clang::BuiltinType::Float:
            result.type = BuiltinType::Float;
            result.bits = 32;
            m_targetType->m_declaredName = "float";
            break;
        case clang::BuiltinType::Double:
            result.type = BuiltinType::Double;
            result.bits = 64;
            m_targetType->m_declaredName = "double";
            break;
        case clang::BuiltinType::LongDouble:
            result.type = BuiltinType::LongDouble;
            result.bits = 80;
            m_targetType->m_declaredName = "long double";
            break;
        case clang::BuiltinType::NullPtr:
            result.type = BuiltinType::Nullptr;
            m_targetType->m_declaredName = "nullptr_t";
            break;
        default:
            result.type = BuiltinType::Extended;
            break;
        }
        result.typeInfo = tp;
        result.kind = tp->getKind();
        m_targetType->m_type = result;
        m_targetType->m_scopedName = m_targetType->m_declaredName;
        m_targetType->m_fullQualifiedName = m_targetType->m_declaredName;

        return true;
    }

    bool VisitElaboratedType(const clang::ElaboratedType* tp)
    {
        return VisitQualType(tp->isSugared() ? tp->desugar() : tp->getNamedType());
    }

    bool VisitReferenceType(const clang::ReferenceType* tp)
    {
        if (tp->isSpelledAsLValue())
            m_targetType->m_isReference = true;
        else
            m_targetType->m_isRVReference = true;

        return VisitQualType(tp->getPointeeType());
    }

    bool VisitDecayedType(const clang::DecayedType* tp)
    {
        return VisitQualType(tp->getOriginalType());
    }

    bool VisitPointerType(const clang::PointerType* tp)
    {
        m_targetType->m_pointingLevels ++;
        return VisitQualType(tp->getPointeeType());
    }

    bool VisitTypedefType(const clang::TypedefType* tp)
    {
        return VisitQualType(tp->getDecl()->getUnderlyingType());
    }

    bool VisitRecordType(const clang::RecordType* tp)
    {
        RecordType result;
        result.decl = tp->getDecl();
        FillDeclDependentFields(result.decl);
        m_targetType->m_type = result;
        return true;
    }

    bool VisitDecltypeType(const clang::DecltypeType* tp)
    {
        DecltypeType result;
        result.declType = tp;
        result.declTypeExpr = tp->getUnderlyingExpr();
        m_targetType->m_type = result;
        return true;
    }

    bool VisitTemplateSpecializationType(const clang::TemplateSpecializationType* tp)
    {
        WellKnownType result;

        clang::TemplateName tplName = tp->getTemplateName();
        if (tplName.getKind() != clang::TemplateName::Template)
            return false;
        const clang::TemplateDecl* tplDecl = tplName.getAsTemplateDecl();
        result.decl = tplDecl;
        FillDeclDependentFields(tplDecl);

        if (tp->isTypeAlias())
            result.aliasedType = TypeInfo::Create(tp->getAliasedType(), m_astContext);

        for (const clang::TemplateArgument& tplArg : tp->template_arguments())
        {
            TemplateType::TplArg argInfo;
            switch (tplArg.getKind())
            {
            case clang::TemplateArgument::Null:
                argInfo = TemplateType::GenericArg{TemplateType::NullTplArg};
                break;
            case clang::TemplateArgument::Type:
                argInfo = TypeInfo::Create(tplArg.getAsType(), m_astContext);
                break;
            case clang::TemplateArgument::Declaration:
                argInfo = tplArg.getAsDecl();
                break;
            case clang::TemplateArgument::NullPtr:
                argInfo = TemplateType::GenericArg{TemplateType::NullPtrTplArg};
                break;
            case clang::TemplateArgument::Integral:
                argInfo = ConvertAPSInt(tplArg.getAsIntegral()).AsSigned();
                break;
            case clang::TemplateArgument::Template:
                argInfo = TemplateType::GenericArg{TemplateType::TemplateTplArg};
                break;
            case clang::TemplateArgument::TemplateExpansion:
                argInfo = TemplateType::GenericArg{TemplateType::TemplateExpansionTplArg};
                break;
            case clang::TemplateArgument::Expression:
            {
                clang::Expr::EvalResult value;
                if (tplArg.getAsExpr()->EvaluateAsInt(value, *m_astContext))
                    argInfo = ConvertAPSInt(value.Val.getInt()).AsSigned();
                else
                    argInfo = TemplateType::GenericArg{TemplateType::UnknownTplArg};

                break;
            }
            case clang::TemplateArgument::Pack:
                argInfo = TemplateType::GenericArg{TemplateType::PackTplArg};
                break;
            }
            result.arguments.push_back(argInfo);
        }

        bool isWellKnownType = DetectWellKnownType(result);

        if (isWellKnownType)
            m_targetType->m_type = std::move(result);
        else
            m_targetType->m_type = static_cast<TemplateType&&>(std::move(result));

        return true;
    }

    bool VisitTemplateTypeParmType(const clang::TemplateTypeParmType* tp)
    {
        TemplateParamType result;
        result.decl = tp->getDecl();
        result.isPack = tp->isParameterPack();
        m_targetType->m_type = result;
        return true;
    }

    bool DetectWellKnownType(WellKnownType& typeInfo)
    {
        bool result = true;
        std::string fqName = m_targetType->m_fullQualifiedName;
        boost::algorithm::replace_all(fqName, "::__cxx11", "");
        boost::algorithm::replace_all(fqName, "::__cxx14", "");
        boost::algorithm::replace_all(fqName, "::__cxx17", "");
        if (fqName == "std::basic_string")
        {
            const BuiltinType* charType = typeInfo.GetTypeArg(0)->getAsBuiltin();
            if (charType->type == BuiltinType::Char)
                typeInfo.type = WellKnownType::StdString;
            else if (charType->type == BuiltinType::WChar)
                typeInfo.type = WellKnownType::StdWstring;
            else
                result = false;
        }
        else if (fqName == "std::vector")
            typeInfo.type = WellKnownType::StdVector;
        else if (fqName == "std::array")
            typeInfo.type = WellKnownType::StdArray;
        else if (fqName == "std::list")
            typeInfo.type = WellKnownType::StdList;
        else if (fqName == "std::deque")
            typeInfo.type = WellKnownType::StdDeque;
        else if (fqName == "std::map")
            typeInfo.type = WellKnownType::StdMap;
        else if (fqName == "std::set")
            typeInfo.type = WellKnownType::StdSet;
        else if (fqName == "std::unordered_map")
            typeInfo.type = WellKnownType::StdUnorderedMap;
        else if (fqName == "std::unordered_set")
            typeInfo.type = WellKnownType::StdUnorderedSet;
        else if (fqName == "std::shared_ptr")
            typeInfo.type = WellKnownType::StdSharedPtr;
        else if (fqName == "std::unique_ptr")
            typeInfo.type = WellKnownType::StdUniquePtr;
        else if (fqName == "std::optional")
            typeInfo.type = WellKnownType::StdOptional;
        else
            result = false;

        return result;
    }

    bool VisitEnumType(const clang::EnumType* tp)
    {
        EnumType result;
        result.decl = tp->getDecl();
        FillDeclDependentFields(result.decl);
        m_targetType->m_type = result;
        return true;
    }

    bool VisitConstantArrayType(const clang::ConstantArrayType* tp)
    {
        ArrayType result;
        result.dims.push_back(ConvertAPInt(tp->getSize()).AsUnsigned());
        TypeInfoPtr itemType = TypeInfo::Create(tp->getElementType(), m_astContext);
        if (itemType->getAsArrayType() != nullptr)
        {
            const ArrayType* subArray = itemType->getAsArrayType();
            result.dims.insert(result.dims.end(), subArray->dims.begin(), subArray->dims.end());
            itemType = subArray->itemType;
        }
        result.itemType = itemType;
        m_targetType->m_type = result;
        return true;
    }

    bool VisitQualType(const clang::QualType& qt)
    {
        m_targetType->m_isConst |= qt.isConstQualified();
        m_targetType->m_isVolatile |= qt.isVolatileQualified();

        return Visit(qt.getTypePtr());
    }

private:
    void FillDeclDependentFields(const clang::NamedDecl* decl)
    {
        NamedDeclInfo declInfo;
        AstReflector::SetupNamedDeclInfo(decl, &declInfo, m_astContext);
        m_targetType->m_declaredName = declInfo.name;
        m_targetType->m_scopedName = declInfo.scopeSpecifier.empty() ? declInfo.name : declInfo.scopeSpecifier + "::" + declInfo.name;
        m_targetType->m_fullQualifiedName = declInfo.namespaceQualifier.empty() ? m_targetType->m_scopedName : declInfo.namespaceQualifier + "::" + m_targetType->m_scopedName;
    }

private:
    TypeInfo* m_targetType;
    const clang::ASTContext* m_astContext;
};

TypeInfo::TypeInfo()
{

}

TypeInfo::TypeDescr TypeInfo::getTypeDescr() const
{
    TypeDescr result;

    auto getScope = [](const std::string& full, const std::string& name)
    {
        auto range = boost::algorithm::find_last(full, name);
        if (range.begin() == full.begin())
            return std::string();

        return full.substr(0, range.begin() - full.begin() - 2);
    };

    result.name = m_declaredName;
    result.scopeSpec = getScope(m_scopedName, m_declaredName);
    result.namespaceQual = getScope(m_fullQualifiedName, m_scopedName);
    result.isConst = m_isConst;
    result.isReference = m_isReference;
    result.isRVReference = m_isRVReference;
    result.isVolatile = m_isVolatile;
    result.pointingLevels = m_pointingLevels;
    result.type = m_type;

    return result;
}

TypeInfoPtr TypeInfo::Create(const clang::QualType& qt, const clang::ASTContext* astContext)
{
    TypeInfoPtr result = std::make_shared<TypeInfo>();

    result->m_printedName = EntityToString(&qt, astContext);
    result->m_typeDecl= qt.getTypePtr();

    TypeUnwrapper visitor(result.get(), astContext);
    bool unwrapped = visitor.VisitQualType(qt);

    if (!unwrapped) // result->m_type.empty())
    {
        result->m_typeDecl->dump();
        std::clog << std::endl;
    }
    // std::clog << "### Unwrapped type: " << result->getFullQualifiedName() << ", " << result << std::endl;

    return result;
}

TypeInfoPtr TypeInfo::Create(const TypeInfo::TypeDescr& descr)
{
    TypeInfoPtr result = std::make_shared<TypeInfo>();

    result->m_type = descr.type;
    result->m_declaredName = descr.name;
    result->m_scopedName = descr.scopeSpec.empty() ? descr.name : descr.scopeSpec + "::" + descr.name;
    result->m_fullQualifiedName = descr.namespaceQual.empty() ? result->m_scopedName : descr.namespaceQual + "::" + result->m_scopedName;
    result->m_isConst = descr.isConst;
    result->m_isReference = descr.isReference;
    result->m_isRVReference = descr.isRVReference;
    result->m_isVolatile = descr.isVolatile;
    result->m_pointingLevels = descr.pointingLevels;

    std::ostringstream os;
    if (descr.isConst)
        os << "const ";
    if (descr.isVolatile)
        os << "volatile ";
    os << result->m_fullQualifiedName;

    if (descr.isReference)
        os << "&";
    else if (descr.isRVReference)
        os << "&&";
    else for(int i = 0; i < descr.pointingLevels; ++ i)
        os << "*";

    result->m_printedName = os.str();

    return result;
}

} // reflection

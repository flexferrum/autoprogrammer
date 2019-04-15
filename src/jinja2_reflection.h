#ifndef JINJA2_REFLECTION_H
#define JINJA2_REFLECTION_H

#include "ast_reflector.h"
#include <jinja2cpp/reflected_value.h>


namespace jinja2
{
#define DUMMY_REFLECTOR(TypeName) \
    template <> \
    struct TypeReflection<TypeName> \
    : TypeReflected<TypeName> { \
    static auto &GetAccessors() { \
    static std::unordered_map<std::string, FieldAccessor> accessors = { \
}; \
    \
    return accessors; \
} \
}

DUMMY_REFLECTOR(clang::ParmVarDecl);
DUMMY_REFLECTOR(clang::CXXMethodDecl);
DUMMY_REFLECTOR(clang::FieldDecl);
DUMMY_REFLECTOR(clang::CXXRecordDecl);
DUMMY_REFLECTOR(reflection::ClassInfo::InnerDeclInfo::DeclType);
DUMMY_REFLECTOR(clang::EnumConstantDecl);
DUMMY_REFLECTOR(clang::EnumDecl);
DUMMY_REFLECTOR(clang::NamespaceDecl);
DUMMY_REFLECTOR(clang::BuiltinType);
DUMMY_REFLECTOR(clang::RecordDecl);
DUMMY_REFLECTOR(clang::NamedDecl);
DUMMY_REFLECTOR(clang::Type);

namespace detail {
template <> struct Reflector<reflection::BuiltinType::Types> {
    static auto Create(reflection::BuiltinType::Types val) {
        switch (val) {
        case reflection::BuiltinType::Unspecified:
            return Value(std::string("Unspecified"));
        case reflection::BuiltinType::Void:
            return Value(std::string("Void"));
        case reflection::BuiltinType::Bool:
            return Value(std::string("Bool"));
        case reflection::BuiltinType::Char:
            return Value(std::string("Char"));
        case reflection::BuiltinType::WChar:
            return Value(std::string("WChar"));
        case reflection::BuiltinType::Char16:
            return Value(std::string("Char16"));
        case reflection::BuiltinType::Char32:
            return Value(std::string("Char32"));
        case reflection::BuiltinType::Short:
            return Value(std::string("Short"));
        case reflection::BuiltinType::Int:
            return Value(std::string("Int"));
        case reflection::BuiltinType::Long:
            return Value(std::string("Long"));
        case reflection::BuiltinType::LongLong:
            return Value(std::string("LongLong"));
        case reflection::BuiltinType::Float:
            return Value(std::string("Float"));
        case reflection::BuiltinType::Double:
            return Value(std::string("Double"));
        case reflection::BuiltinType::LongDouble:
            return Value(std::string("LongDouble"));
        case reflection::BuiltinType::Nullptr:
            return Value(std::string("Nullptr"));
        case reflection::BuiltinType::Extended:
            return Value(std::string("Extended"));
        }

        return Value();
    }
    static auto CreateFromPtr(const reflection::BuiltinType::Types *val) {
        return Create(*val);
    }
};
template <> struct Reflector<reflection::BuiltinType::SignType> {
    static auto Create(reflection::BuiltinType::SignType val) {
        switch (val) {
        case reflection::BuiltinType::Signed:
            return Value(std::string("Signed"));
        case reflection::BuiltinType::Unsigned:
            return Value(std::string("Unsigned"));
        case reflection::BuiltinType::NoSign:
            return Value(std::string("NoSign"));
        }

        return Value();
    }
    static auto CreateFromPtr(const reflection::BuiltinType::SignType *val) {
        return Create(*val);
    }
};
template <> struct Reflector<clang::BuiltinType::Kind> {
    static auto Create(clang::BuiltinType::Kind val) {
        switch (val) {
        case clang::BuiltinType::OCLImage1dRO:
            return Value(std::string("OCLImage1dRO"));
        case clang::BuiltinType::OCLImage1dArrayRO:
            return Value(std::string("OCLImage1dArrayRO"));
        case clang::BuiltinType::OCLImage1dBufferRO:
            return Value(std::string("OCLImage1dBufferRO"));
        case clang::BuiltinType::OCLImage2dRO:
            return Value(std::string("OCLImage2dRO"));
        case clang::BuiltinType::OCLImage2dArrayRO:
            return Value(std::string("OCLImage2dArrayRO"));
        case clang::BuiltinType::OCLImage2dDepthRO:
            return Value(std::string("OCLImage2dDepthRO"));
        case clang::BuiltinType::OCLImage2dArrayDepthRO:
            return Value(std::string("OCLImage2dArrayDepthRO"));
        case clang::BuiltinType::OCLImage2dMSAARO:
            return Value(std::string("OCLImage2dMSAARO"));
        case clang::BuiltinType::OCLImage2dArrayMSAARO:
            return Value(std::string("OCLImage2dArrayMSAARO"));
        case clang::BuiltinType::OCLImage2dMSAADepthRO:
            return Value(std::string("OCLImage2dMSAADepthRO"));
        case clang::BuiltinType::OCLImage2dArrayMSAADepthRO:
            return Value(std::string("OCLImage2dArrayMSAADepthRO"));
        case clang::BuiltinType::OCLImage3dRO:
            return Value(std::string("OCLImage3dRO"));
        case clang::BuiltinType::OCLImage1dWO:
            return Value(std::string("OCLImage1dWO"));
        case clang::BuiltinType::OCLImage1dArrayWO:
            return Value(std::string("OCLImage1dArrayWO"));
        case clang::BuiltinType::OCLImage1dBufferWO:
            return Value(std::string("OCLImage1dBufferWO"));
        case clang::BuiltinType::OCLImage2dWO:
            return Value(std::string("OCLImage2dWO"));
        case clang::BuiltinType::OCLImage2dArrayWO:
            return Value(std::string("OCLImage2dArrayWO"));
        case clang::BuiltinType::OCLImage2dDepthWO:
            return Value(std::string("OCLImage2dDepthWO"));
        case clang::BuiltinType::OCLImage2dArrayDepthWO:
            return Value(std::string("OCLImage2dArrayDepthWO"));
        case clang::BuiltinType::OCLImage2dMSAAWO:
            return Value(std::string("OCLImage2dMSAAWO"));
        case clang::BuiltinType::OCLImage2dArrayMSAAWO:
            return Value(std::string("OCLImage2dArrayMSAAWO"));
        case clang::BuiltinType::OCLImage2dMSAADepthWO:
            return Value(std::string("OCLImage2dMSAADepthWO"));
        case clang::BuiltinType::OCLImage2dArrayMSAADepthWO:
            return Value(std::string("OCLImage2dArrayMSAADepthWO"));
        case clang::BuiltinType::OCLImage3dWO:
            return Value(std::string("OCLImage3dWO"));
        case clang::BuiltinType::OCLImage1dRW:
            return Value(std::string("OCLImage1dRW"));
        case clang::BuiltinType::OCLImage1dArrayRW:
            return Value(std::string("OCLImage1dArrayRW"));
        case clang::BuiltinType::OCLImage1dBufferRW:
            return Value(std::string("OCLImage1dBufferRW"));
        case clang::BuiltinType::OCLImage2dRW:
            return Value(std::string("OCLImage2dRW"));
        case clang::BuiltinType::OCLImage2dArrayRW:
            return Value(std::string("OCLImage2dArrayRW"));
        case clang::BuiltinType::OCLImage2dDepthRW:
            return Value(std::string("OCLImage2dDepthRW"));
        case clang::BuiltinType::OCLImage2dArrayDepthRW:
            return Value(std::string("OCLImage2dArrayDepthRW"));
        case clang::BuiltinType::OCLImage2dMSAARW:
            return Value(std::string("OCLImage2dMSAARW"));
        case clang::BuiltinType::OCLImage2dArrayMSAARW:
            return Value(std::string("OCLImage2dArrayMSAARW"));
        case clang::BuiltinType::OCLImage2dMSAADepthRW:
            return Value(std::string("OCLImage2dMSAADepthRW"));
        case clang::BuiltinType::OCLImage2dArrayMSAADepthRW:
            return Value(std::string("OCLImage2dArrayMSAADepthRW"));
        case clang::BuiltinType::OCLImage3dRW:
            return Value(std::string("OCLImage3dRW"));
        case clang::BuiltinType::Void:
            return Value(std::string("Void"));
        case clang::BuiltinType::Bool:
            return Value(std::string("Bool"));
        case clang::BuiltinType::Char_U:
            return Value(std::string("Char_U"));
        case clang::BuiltinType::UChar:
            return Value(std::string("UChar"));
        case clang::BuiltinType::WChar_U:
            return Value(std::string("WChar_U"));
        case clang::BuiltinType::Char16:
            return Value(std::string("Char16"));
        case clang::BuiltinType::Char32:
            return Value(std::string("Char32"));
        case clang::BuiltinType::UShort:
            return Value(std::string("UShort"));
        case clang::BuiltinType::UInt:
            return Value(std::string("UInt"));
        case clang::BuiltinType::ULong:
            return Value(std::string("ULong"));
        case clang::BuiltinType::ULongLong:
            return Value(std::string("ULongLong"));
        case clang::BuiltinType::UInt128:
            return Value(std::string("UInt128"));
        case clang::BuiltinType::Char_S:
            return Value(std::string("Char_S"));
        case clang::BuiltinType::SChar:
            return Value(std::string("SChar"));
        case clang::BuiltinType::WChar_S:
            return Value(std::string("WChar_S"));
        case clang::BuiltinType::Short:
            return Value(std::string("Short"));
        case clang::BuiltinType::Int:
            return Value(std::string("Int"));
        case clang::BuiltinType::Long:
            return Value(std::string("Long"));
        case clang::BuiltinType::LongLong:
            return Value(std::string("LongLong"));
        case clang::BuiltinType::Int128:
            return Value(std::string("Int128"));
        case clang::BuiltinType::Half:
            return Value(std::string("Half"));
        case clang::BuiltinType::Float:
            return Value(std::string("Float"));
        case clang::BuiltinType::Double:
            return Value(std::string("Double"));
        case clang::BuiltinType::LongDouble:
            return Value(std::string("LongDouble"));
        case clang::BuiltinType::Float128:
            return Value(std::string("Float128"));
        case clang::BuiltinType::NullPtr:
            return Value(std::string("NullPtr"));
        case clang::BuiltinType::ObjCId:
            return Value(std::string("ObjCId"));
        case clang::BuiltinType::ObjCClass:
            return Value(std::string("ObjCClass"));
        case clang::BuiltinType::ObjCSel:
            return Value(std::string("ObjCSel"));
        case clang::BuiltinType::OCLSampler:
            return Value(std::string("OCLSampler"));
        case clang::BuiltinType::OCLEvent:
            return Value(std::string("OCLEvent"));
        case clang::BuiltinType::OCLClkEvent:
            return Value(std::string("OCLClkEvent"));
        case clang::BuiltinType::OCLQueue:
            return Value(std::string("OCLQueue"));
        case clang::BuiltinType::OCLReserveID:
            return Value(std::string("OCLReserveID"));
        case clang::BuiltinType::Dependent:
            return Value(std::string("Dependent"));
        case clang::BuiltinType::Overload:
            return Value(std::string("Overload"));
        case clang::BuiltinType::BoundMember:
            return Value(std::string("BoundMember"));
        case clang::BuiltinType::PseudoObject:
            return Value(std::string("PseudoObject"));
        case clang::BuiltinType::UnknownAny:
            return Value(std::string("UnknownAny"));
        case clang::BuiltinType::BuiltinFn:
            return Value(std::string("BuiltinFn"));
        case clang::BuiltinType::ARCUnbridgedCast:
            return Value(std::string("ARCUnbridgedCast"));
        case clang::BuiltinType::OMPArraySection:
            return Value(std::string("OMPArraySection"));
        }

        return Value();
    }
    static auto CreateFromPtr(const clang::BuiltinType::Kind *val) {
        return Create(*val);
    }
};
template <> struct Reflector<reflection::TemplateType::TplArgKind> {
    static auto Create(reflection::TemplateType::TplArgKind val) {
        switch (val) {
        case reflection::TemplateType::UnknownTplArg:
            return Value(std::string("UnknownTplArg"));
        case reflection::TemplateType::NullTplArg:
            return Value(std::string("NullTplArg"));
        case reflection::TemplateType::NullPtrTplArg:
            return Value(std::string("NullPtrTplArg"));
        case reflection::TemplateType::TemplateTplArg:
            return Value(std::string("TemplateTplArg"));
        case reflection::TemplateType::TemplateExpansionTplArg:
            return Value(std::string("TemplateExpansionTplArg"));
        case reflection::TemplateType::PackTplArg:
            return Value(std::string("PackTplArg"));
        }

        return Value();
    }
    static auto CreateFromPtr(const reflection::TemplateType::TplArgKind *val) {
        return Create(*val);
    }
};
template <> struct Reflector<reflection::WellKnownType::Types> {
    static auto Create(reflection::WellKnownType::Types val) {
        switch (val) {
        case reflection::WellKnownType::StdArray:
            return Value(std::string("StdArray"));
        case reflection::WellKnownType::StdString:
            return Value(std::string("StdString"));
        case reflection::WellKnownType::StdWstring:
            return Value(std::string("StdWstring"));
        case reflection::WellKnownType::StdVector:
            return Value(std::string("StdVector"));
        case reflection::WellKnownType::StdList:
            return Value(std::string("StdList"));
        case reflection::WellKnownType::StdDeque:
            return Value(std::string("StdDeque"));
        case reflection::WellKnownType::StdMap:
            return Value(std::string("StdMap"));
        case reflection::WellKnownType::StdSet:
            return Value(std::string("StdSet"));
        case reflection::WellKnownType::StdUnorderedMap:
            return Value(std::string("StdUnorderedMap"));
        case reflection::WellKnownType::StdUnorderedSet:
            return Value(std::string("StdUnorderedSet"));
        case reflection::WellKnownType::StdSharedPtr:
            return Value(std::string("StdSharedPtr"));
        case reflection::WellKnownType::StdUniquePtr:
            return Value(std::string("StdUniquePtr"));
        case reflection::WellKnownType::StdOptional:
            return Value(std::string("StdOptional"));
        case reflection::WellKnownType::BoostOptional:
            return Value(std::string("BoostOptional"));
        case reflection::WellKnownType::BoostVariant:
            return Value(std::string("BoostVariant"));
        }

        return Value();
    }
    static auto CreateFromPtr(const reflection::WellKnownType::Types *val) {
        return Create(*val);
    }
};
template <> struct Reflector<reflection::AccessType> {
    static auto Create(reflection::AccessType val) {
        switch (val) {
        case reflection::AccessType::Public:
            return Value(std::string("Public"));
        case reflection::AccessType::Protected:
            return Value(std::string("Protected"));
        case reflection::AccessType::Private:
            return Value(std::string("Private"));
        case reflection::AccessType::Undefined:
            return Value(std::string("Undefined"));
        }

        return Value();
    }
    static auto CreateFromPtr(const reflection::AccessType *val) {
        return Create(*val);
    }
};
template <> struct Reflector<reflection::AssignmentOperType> {
    static auto Create(reflection::AssignmentOperType val) {
        switch (val) {
        case reflection::AssignmentOperType::None:
            return Value(std::string("None"));
        case reflection::AssignmentOperType::Generic:
            return Value(std::string("Generic"));
        case reflection::AssignmentOperType::Copy:
            return Value(std::string("Copy"));
        case reflection::AssignmentOperType::Move:
            return Value(std::string("Move"));
        }

        return Value();
    }
    static auto CreateFromPtr(const reflection::AssignmentOperType *val) {
        return Create(*val);
    }
};
template <> struct Reflector<reflection::ConstructorType> {
    static auto Create(reflection::ConstructorType val) {
        switch (val) {
        case reflection::ConstructorType::None:
            return Value(std::string("None"));
        case reflection::ConstructorType::Generic:
            return Value(std::string("Generic"));
        case reflection::ConstructorType::Default:
            return Value(std::string("Default"));
        case reflection::ConstructorType::Copy:
            return Value(std::string("Copy"));
        case reflection::ConstructorType::Move:
            return Value(std::string("Move"));
        case reflection::ConstructorType::Convert:
            return Value(std::string("Convert"));
        }

        return Value();
    }
    static auto CreateFromPtr(const reflection::ConstructorType *val) {
        return Create(*val);
    }
};
} // namespace detail


template <>
struct TypeReflection<reflection::NoType> : TypeReflected<reflection::NoType> {
    static auto &GetAccessors() {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            {"isNoType", [](const reflection::NoType&) { return true; }}
        };

        return accessors;
    }
};
template <>
struct TypeReflection<reflection::BuiltinType>
        : TypeReflected<reflection::BuiltinType> {
    static auto &GetAccessors() {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            {"bits",
             [](const reflection::BuiltinType &obj) { return Reflect(obj.bits); }},
            {"isSigned",
             [](const reflection::BuiltinType &obj) {
                 return Reflect(obj.isSigned);
             }},
            {"kind",
             [](const reflection::BuiltinType &obj) { return Reflect(obj.kind); }},
            {"type",
             [](const reflection::BuiltinType &obj) { return Reflect(obj.type); }},
            {"isBuiltinType", [](const reflection::BuiltinType&) { return true; }}
        };

        return accessors;
    }
};
template <>
struct TypeReflection<reflection::RecordType>
        : TypeReflected<reflection::RecordType> {
    static auto &GetAccessors() {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            {"isRecordType", [](const reflection::RecordType&) { return true; }}
        };

        return accessors;
    }
};
template <>
struct TypeReflection<reflection::TemplateType>
        : TypeReflected<reflection::TemplateType> {
    static auto &GetAccessors() {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            {"aliasedType",
             [](const reflection::TemplateType &obj) {
                 return Reflect(obj.aliasedType);
             }},
            //        {"arguments",
            //         [](const reflection::TemplateType &obj) {
            //           return Reflect(obj.arguments);
            //         }},
            {"isTemplateType", [](const reflection::TemplateType&) { return true; }}
        };

        return accessors;
    }
};
template <>
struct TypeReflection<reflection::TemplateType::GenericArg>
        : TypeReflected<reflection::TemplateType::GenericArg> {
    static auto &GetAccessors() {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            {"kind",
             [](const reflection::TemplateType::GenericArg &obj) {
                 return Reflect(obj.kind);
             }},
        };

        return accessors;
    }
};
template <>
struct TypeReflection<reflection::WellKnownType>
        : TypeReflected<reflection::WellKnownType> {
    static auto &GetAccessors() {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            {"aliasedType",
             [](const reflection::WellKnownType &obj) {
                 return Reflect(obj.aliasedType);
             }},
            //        {"arguments",
            //         [](const reflection::WellKnownType &obj) {
            //           return Reflect(obj.arguments);
            //         }},
            {"type",
             [](const reflection::WellKnownType &obj) {
                 return Reflect(obj.type);
             }},
            {"isWellKnownType", [](const reflection::WellKnownType&) { return true; }}
        };

        return accessors;
    }
};
template <>
struct TypeReflection<reflection::ArrayType>
        : TypeReflected<reflection::ArrayType> {
    static auto &GetAccessors() {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            {"dims",
             [](const reflection::ArrayType &obj) { return Reflect(obj.dims); }},
            {"itemType",
             [](const reflection::ArrayType &obj) {
                 return Reflect(obj.itemType);
             }},
            {"isArrayType", [](const reflection::ArrayType&) { return true; }}
        };

        return accessors;
    }
};
template <>
struct TypeReflection<reflection::EnumType>
        : TypeReflected<reflection::EnumType> {
    static auto &GetAccessors() {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            {"isEnumType", [](const reflection::EnumType&) { return true; }}
        };

        return accessors;
    }
};
template <>
struct TypeReflection<reflection::DecltypeType>
        : TypeReflected<reflection::DecltypeType> {
    static auto &GetAccessors() {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            {"isDecltypeType", [](const reflection::DecltypeType&) { return true; }}
        };

        return accessors;
    }
};
template <>
struct TypeReflection<reflection::TemplateParamType>
        : TypeReflected<reflection::TemplateParamType> {
    static auto &GetAccessors() {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            {"isPack",
             [](const reflection::TemplateParamType &obj) {
                 return Reflect(obj.isPack);
             }},
            {"isTemplateParamType", [](const reflection::TemplateParamType&) { return true; }}
        };

        return accessors;
    }
};
template <>
struct TypeReflection<reflection::TypeInfo>
        : TypeReflected<reflection::TypeInfo> {
    static auto &GetAccessors() {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            {"canBeMoved",
             [](const reflection::TypeInfo &obj) {
                 return Reflect(obj.canBeMoved());
             }},
            {"declaredName",
             [](const reflection::TypeInfo &obj) {
                 return Reflect(obj.getDeclaredName());
             }},
            {"fullQualifiedName",
             [](const reflection::TypeInfo &obj) {
                 return Reflect(obj.getFullQualifiedName());
             }},
            {"isConst",
             [](const reflection::TypeInfo &obj) {
                 return Reflect(obj.getIsConst());
             }},
            {"isNoType",
             [](const reflection::TypeInfo &obj) {
                 return Reflect(obj.isNoType());
             }},
            {"isReference",
             [](const reflection::TypeInfo &obj) {
                 return Reflect(obj.getIsReference());
             }},
            {"isRVReference",
             [](const reflection::TypeInfo &obj) {
                 return Reflect(obj.getIsRVReference());
             }},
            {"isVolatile",
             [](const reflection::TypeInfo &obj) {
                 return Reflect(obj.getIsVolatile());
             }},
            {"pointingLevels",
             [](const reflection::TypeInfo &obj) {
                 return Reflect(obj.getPointingLevels());
             }},
            {"printedName",
             [](const reflection::TypeInfo &obj) {
                 return Reflect(obj.getPrintedName());
             }},
            {"scopedName",
             [](const reflection::TypeInfo &obj) {
                 return Reflect(obj.getScopedName());
             }},
            {"type",
             [](const reflection::TypeInfo &obj) {
                 return boost::apply_visitor(InnerTypeReflector(), obj.GetType());
             }}
        };

        return accessors;
    }

    struct InnerTypeReflector : public boost::static_visitor<Value>
    {
        template<typename T>
        auto operator() (T&& val) const
        {
            return Reflect(std::forward<T>(val));
        }
    };
};
//template <>
//struct TypeReflection<reflection::TypeInfo::canBeMoved::Visitor>
//    : TypeReflected<reflection::TypeInfo::canBeMoved::Visitor> {
//  static auto &GetAccessors() {
//    static std::unordered_map<std::string, FieldAccessor> accessors = {
//        {"m_type",
//         [](const reflection::TypeInfo::canBeMoved::Visitor &obj) {
//           return Reflect(obj.m_type);
//         }},
//    };

//    return accessors;
//  }
//};

template <>
struct TypeReflection<reflection::NamedDeclInfo>
        : TypeReflected<reflection::NamedDeclInfo> {
    static auto &GetAccessors() {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            {"fullQualifiedName",
             [](const reflection::NamedDeclInfo &obj) {
                 return Reflect(obj.GetFullQualifiedName());
             }},
            {"name",
             [](const reflection::NamedDeclInfo &obj) {
                 return Reflect(obj.name);
             }},
            {"namespaceQualifier",
             [](const reflection::NamedDeclInfo &obj) {
                 return Reflect(obj.namespaceQualifier);
             }},
            {"scopedName",
             [](const reflection::NamedDeclInfo &obj) {
                 return Reflect(obj.GetScopedName());
             }},
            {"scopeSpecifier",
             [](const reflection::NamedDeclInfo &obj) {
                 return Reflect(obj.scopeSpecifier);
             }},
        };

        return accessors;
    }
};
template <>
struct TypeReflection<reflection::SourceLocation>
        : TypeReflected<reflection::SourceLocation> {
    static auto &GetAccessors() {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            {"column",
             [](const reflection::SourceLocation &obj) {
                 return Reflect(obj.column);
             }},
            {"fileName",
             [](const reflection::SourceLocation &obj) {
                 return Reflect(obj.fileName);
             }},
            {"line",
             [](const reflection::SourceLocation &obj) {
                 return Reflect(obj.line);
             }},
        };

        return accessors;
    }
};
template <>
struct TypeReflection<reflection::LocationInfo>
        : TypeReflected<reflection::LocationInfo> {
    static auto &GetAccessors() {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            {"location",
             [](const reflection::LocationInfo &obj) {
                 return Reflect(obj.location);
             }},
        };

        return accessors;
    }
};
template <>
struct TypeReflection<reflection::MethodParamInfo>
        : TypeReflected<reflection::MethodParamInfo> {
    static auto &GetAccessors() {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            {"fullDecl",
             [](const reflection::MethodParamInfo &obj) {
                 return Reflect(obj.fullDecl);
             }},
            {"name",
             [](const reflection::MethodParamInfo &obj) {
                 return Reflect(obj.name);
             }},
            {"type",
             [](const reflection::MethodParamInfo &obj) {
                 return Reflect(obj.type);
             }},
        };

        return accessors;
    }
};
template <>
struct TypeReflection<reflection::TemplateParamInfo>
        : TypeReflected<reflection::TemplateParamInfo> {
    static auto &GetAccessors() {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            {"tplDeclName",
             [](const reflection::TemplateParamInfo &obj) {
                 return Reflect(obj.tplDeclName);
             }},
            {"tplDeclName",
             [](const reflection::TemplateParamInfo &obj) {
                 return Reflect(obj.tplDeclName);
             }},
            {"kind",
             [](const reflection::TemplateParamInfo &obj) {
                 return Reflect(obj.kind);
             }},
            {"isParamPack",
             [](const reflection::TemplateParamInfo &obj) {
                 return Reflect(obj.isParamPack);
             }},
        };

        return accessors;
    }
};
template <>
struct TypeReflection<reflection::MethodInfo>
        : TypeReflected<reflection::MethodInfo> {
    static auto &GetAccessors() {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            {"accessType",
             [](const reflection::MethodInfo &obj) {
                 return Reflect(obj.accessType);
             }},
            {"assignmentOperType",
             [](const reflection::MethodInfo &obj) {
                 return Reflect(obj.assignmentOperType);
             }},
            {"constructorType",
             [](const reflection::MethodInfo &obj) {
                 return Reflect(obj.constructorType);
             }},
            {"declLocation",
             [](const reflection::MethodInfo &obj) {
                 return Reflect(obj.declLocation);
             }},
            {"defLocation",
             [](const reflection::MethodInfo &obj) {
                 return Reflect(obj.defLocation);
             }},
            {"fullPrototype",
             [](const reflection::MethodInfo &obj) {
                 return Reflect(obj.fullPrototype);
             }},
            {"fullQualifiedName",
             [](const reflection::MethodInfo &obj) {
                 return Reflect(obj.GetFullQualifiedName());
             }},
            {"isConst",
             [](const reflection::MethodInfo &obj) {
                 return Reflect(obj.isConst);
             }},
            {"isCtor",
             [](const reflection::MethodInfo &obj) { return Reflect(obj.isCtor); }},
            {"isDeleted",
             [](const reflection::MethodInfo &obj) {
                 return Reflect(obj.isDeleted);
             }},
            {"isDtor",
             [](const reflection::MethodInfo &obj) { return Reflect(obj.isDtor); }},
            {"isExplicitCtor",
             [](const reflection::MethodInfo &obj) {
                 return Reflect(obj.isExplicitCtor);
             }},
            {"isImplicit",
             [](const reflection::MethodInfo &obj) {
                 return Reflect(obj.isImplicit);
             }},
            {"isNoExcept",
             [](const reflection::MethodInfo &obj) {
                 return Reflect(obj.isNoExcept);
             }},
            {"isOperator",
             [](const reflection::MethodInfo &obj) {
                 return Reflect(obj.isOperator);
             }},
            {"isPure",
             [](const reflection::MethodInfo &obj) { return Reflect(obj.isPure); }},
            {"isRVRef",
             [](const reflection::MethodInfo &obj) {
                 return Reflect(obj.isRVRef);
             }},
            {"isStatic",
             [](const reflection::MethodInfo &obj) {
                 return Reflect(obj.isStatic);
             }},
            {"isVirtual",
             [](const reflection::MethodInfo &obj) {
                 return Reflect(obj.isVirtual);
             }},
            {"isTemplate",
             [](const reflection::MethodInfo &obj) {
                 return Reflect(obj.isTemplate());
             }},
            {"isInlined",
             [](const reflection::MethodInfo &obj) {
                 return Reflect(obj.isInlined | obj.isClassScopeInlined);
             }},
            {"isClassScopeInlined",
             [](const reflection::MethodInfo &obj) {
                 return Reflect(obj.isClassScopeInlined);
             }},
            {"isDefined",
             [](const reflection::MethodInfo &obj) {
                 return Reflect(obj.isDefined);
             }},
            {"name",
             [](const reflection::MethodInfo &obj)
             {
                 return Reflect(obj.name);
             }},
            {"body",
             [](const reflection::MethodInfo &obj)
             {
                 return obj.isDefined ? Reflect(obj.body) : Value();
             }},
            {"namespaceQualifier",
             [](const reflection::MethodInfo &obj) {
                 return Reflect(obj.namespaceQualifier);
             }},
            {"params",
             [](const reflection::MethodInfo &obj) { return Reflect(obj.params); }},
            {"tplParams",
             [](const reflection::MethodInfo &obj) { return Reflect(obj.tplParams); }},
            {"returnType",
             [](const reflection::MethodInfo &obj) {
                 return Reflect(obj.returnType);
             }},
            {"returnTypeAsString",
             [](const reflection::MethodInfo &obj) {
                 return Reflect(obj.returnTypeAsString);
             }},
            {"scopedName",
             [](const reflection::MethodInfo &obj) {
                 return Reflect(obj.GetScopedName());
             }},
            {"scopeSpecifier",
             [](const reflection::MethodInfo &obj) {
                 return Reflect(obj.scopeSpecifier);
             }},
        };

        return accessors;
    }
};
template <>
struct TypeReflection<reflection::MemberInfo>
        : TypeReflected<reflection::MemberInfo> {
    static auto &GetAccessors() {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            {"accessType",
             [](const reflection::MemberInfo &obj) {
                 return Reflect(obj.accessType);
             }},
            {"fullQualifiedName",
             [](const reflection::MemberInfo &obj) {
                 return Reflect(obj.GetFullQualifiedName());
             }},
            {"isStatic",
             [](const reflection::MemberInfo &obj) {
                 return Reflect(obj.isStatic);
             }},
            {"location",
             [](const reflection::MemberInfo &obj) {
                 return Reflect(obj.location);
             }},
            {"name",
             [](const reflection::MemberInfo &obj) { return Reflect(obj.name); }},
            {"namespaceQualifier",
             [](const reflection::MemberInfo &obj) {
                 return Reflect(obj.namespaceQualifier);
             }},
            {"scopedName",
             [](const reflection::MemberInfo &obj) {
                 return Reflect(obj.GetScopedName());
             }},
            {"scopeSpecifier",
             [](const reflection::MemberInfo &obj) {
                 return Reflect(obj.scopeSpecifier);
             }},
            {"type",
             [](const reflection::MemberInfo &obj) { return Reflect(obj.type); }},
        };

        return accessors;
    }
};
template <>
struct TypeReflection<reflection::GenericDeclPart>
        : TypeReflected<reflection::GenericDeclPart> {
    static auto &GetAccessors() {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            {"accessType",
             [](const reflection::GenericDeclPart &obj) {
                 return Reflect(obj.accessType);
             }},
            {"content",
             [](const reflection::GenericDeclPart &obj) {
                 return Reflect(obj.content);
             }},
            {"location",
             [](const reflection::GenericDeclPart &obj) {
                 return Reflect(obj.location);
             }}
        };

        return accessors;
    }
};
template <>
struct TypeReflection<reflection::ClassInfo>
        : TypeReflected<reflection::ClassInfo> {
    static auto &GetAccessors() {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            {"baseClasses",
             [](const reflection::ClassInfo &obj) {
                 return Reflect(obj.baseClasses);
             }},
            {"fullQualifiedName",
             [](const reflection::ClassInfo &obj) {
                 return Reflect(obj.GetFullQualifiedName());
             }},
            {"hasDefinition",
             [](const reflection::ClassInfo &obj) {
                 return Reflect(obj.hasDefinition);
             }},
            {"innerDecls",
             [](const reflection::ClassInfo &obj) {
                 return Reflect(obj.innerDecls);
             }},
            {"isAbstract",
             [](const reflection::ClassInfo &obj) {
                 return Reflect(obj.isAbstract);
             }},
            {"isTrivial",
             [](const reflection::ClassInfo &obj) {
                 return Reflect(obj.isTrivial);
             }},
            {"isUnion",
             [](const reflection::ClassInfo &obj) { return Reflect(obj.isUnion); }},
            {"location",
             [](const reflection::ClassInfo &obj) {
                 return Reflect(obj.location);
             }},
            {"members",
             [](const reflection::ClassInfo &obj) { return Reflect(obj.members); }},
            {"genericParts",
             [](const reflection::ClassInfo &obj) { return Reflect(obj.genericParts); }},
            {"methods",
             [](const reflection::ClassInfo &obj) { return Reflect(obj.methods); }},
            {"tplParams",
             [](const reflection::ClassInfo &obj) { return Reflect(obj.templateParams); }},
            {"isTemplate",
             [](const reflection::ClassInfo &obj) { return !obj.templateParams.empty(); }},
            {"name",
             [](const reflection::ClassInfo &obj) { return Reflect(obj.name); }},
            {"namespaceQualifier",
             [](const reflection::ClassInfo &obj) {
                 return Reflect(obj.namespaceQualifier);
             }},
            {"scopedName",
             [](const reflection::ClassInfo &obj) {
                 return Reflect(obj.GetScopedName());
             }},
            {"scopeSpecifier",
             [](const reflection::ClassInfo &obj) {
                 return Reflect(obj.scopeSpecifier);
             }},
        };

        return accessors;
    }
};
template <>
struct TypeReflection<reflection::ClassInfo::BaseInfo>
        : TypeReflected<reflection::ClassInfo::BaseInfo> {
    static auto &GetAccessors() {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            {"accessType",
             [](const reflection::ClassInfo::BaseInfo &obj) {
                 return Reflect(obj.accessType);
             }},
            {"baseClass",
             [](const reflection::ClassInfo::BaseInfo &obj) {
                 return Reflect(obj.baseClass);
             }},
            {"isVirtual",
             [](const reflection::ClassInfo::BaseInfo &obj) {
                 return Reflect(obj.isVirtual);
             }},
        };

        return accessors;
    }
};
template <>
struct TypeReflection<reflection::ClassInfo::InnerDeclInfo>
        : TypeReflected<reflection::ClassInfo::InnerDeclInfo> {
    static auto &GetAccessors() {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            {"acessType",
             [](const reflection::ClassInfo::InnerDeclInfo &obj) {
                 return Reflect(obj.acessType);
             }},
            {"innerDecl",
             [](const reflection::ClassInfo::InnerDeclInfo &obj) {
                 return Reflect(obj.innerDecl);
             }},
        };

        return accessors;
    }
};
template <>
struct TypeReflection<reflection::EnumItemInfo>
        : TypeReflected<reflection::EnumItemInfo> {
    static auto &GetAccessors() {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            {"itemName",
             [](const reflection::EnumItemInfo &obj) {
                 return Reflect(obj.itemName);
             }},
            {"itemValue",
             [](const reflection::EnumItemInfo &obj) {
                 return Reflect(obj.itemValue);
             }},
            {"location",
             [](const reflection::EnumItemInfo &obj) {
                 return Reflect(obj.location);
             }},
        };

        return accessors;
    }
};
template <>
struct TypeReflection<reflection::EnumInfo>
        : TypeReflected<reflection::EnumInfo> {
    static auto &GetAccessors() {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            {"fullQualifiedName",
             [](const reflection::EnumInfo &obj) {
                 return Reflect(obj.GetFullQualifiedName());
             }},
            {"isScoped",
             [](const reflection::EnumInfo &obj) { return Reflect(obj.isScoped); }},
            {"items",
             [](const reflection::EnumInfo &obj) { return Reflect(obj.items); }},
            {"location",
             [](const reflection::EnumInfo &obj) { return Reflect(obj.location); }},
            {"name",
             [](const reflection::EnumInfo &obj) { return Reflect(obj.name); }},
            {"namespaceQualifier",
             [](const reflection::EnumInfo &obj) {
                 return Reflect(obj.namespaceQualifier);
             }},
            {"scopedName",
             [](const reflection::EnumInfo &obj) {
                 return Reflect(obj.GetScopedName());
             }},
            {"scopeSpecifier",
             [](const reflection::EnumInfo &obj) {
                 return Reflect(obj.scopeSpecifier);
             }},
        };

        return accessors;
    }
};
template <>
struct TypeReflection<reflection::NamespaceInfo>
        : TypeReflected<reflection::NamespaceInfo> {
    static auto &GetAccessors() {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            {"classes",
             [](const reflection::NamespaceInfo &obj) {
                 return Reflect(obj.classes);
             }},
            {"enums",
             [](const reflection::NamespaceInfo &obj) {
                 return Reflect(obj.enums);
             }},
            {"fullQualifiedName",
             [](const reflection::NamespaceInfo &obj) {
                 return Reflect(obj.GetFullQualifiedName());
             }},
            {"innerNamespaces",
             [](const reflection::NamespaceInfo &obj) {
                 return Reflect(obj.innerNamespaces);
             }},
            {"isRootNamespace",
             [](const reflection::NamespaceInfo &obj) {
                 return Reflect(obj.isRootNamespace);
             }},
            {"name",
             [](const reflection::NamespaceInfo &obj) {
                 return Reflect(obj.name);
             }},
            {"namespaceQualifier",
             [](const reflection::NamespaceInfo &obj) {
                 return Reflect(obj.namespaceQualifier);
             }},
            {"scopedName",
             [](const reflection::NamespaceInfo &obj) {
                 return Reflect(obj.GetScopedName());
             }},
            {"scopeSpecifier",
             [](const reflection::NamespaceInfo &obj) {
                 return Reflect(obj.scopeSpecifier);
             }},
        };

        return accessors;
    }
};
} // jinja2

#endif // JINJA2_REFLECTION_H

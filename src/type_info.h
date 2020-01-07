#ifndef TYPE_INFO_H
#define TYPE_INFO_H

#include "utils.h"

#include <clang/AST/DeclCXX.h>

#include <boost/variant.hpp>
#include <memory>

namespace reflection
{
class TypeInfo;
using TypeInfoPtr = std::shared_ptr<TypeInfo>;

struct NoType
{
};

struct BuiltinType
{
    enum Types
    {
        Unspecified,
        Void,
        Bool,
        Char,
        WChar,
        Char16,
        Char32,
        Short,
        Int,
        Long,
        LongLong,
        Float,
        Double,
        LongDouble,
        Nullptr,
        Extended
    };

    enum SignType
    {
        Signed,
        Unsigned,
        NoSign
    };

    Types type = Unspecified;
    SignType isSigned = NoSign;
    int bits = 0;
    clang::BuiltinType::Kind kind = clang::BuiltinType::UnknownAny;
    const clang::BuiltinType* typeInfo = nullptr;
};

struct RecordType
{
    const clang::RecordDecl* decl = nullptr;
};

struct TemplateType
{
    enum TplArgKind
    {
        UnknownTplArg,
        NullTplArg,
        NullPtrTplArg,
        TemplateTplArg,
        TemplateExpansionTplArg,
        PackTplArg
    };

    struct GenericArg
    {
        GenericArg(TplArgKind k = UnknownTplArg)
            : kind(k)
        {
        }

        TplArgKind kind;
    };

    using TplArg = boost::variant<GenericArg, TypeInfoPtr, const clang::ValueDecl*, int64_t>;
    std::vector<TplArg> arguments;
    TypeInfoPtr aliasedType;
    const clang::NamedDecl* decl;

    auto GetTypeArg(int idx) const
    {
        const TypeInfoPtr* result = boost::get<TypeInfoPtr>(&arguments[idx]);
        return result ? *result : TypeInfoPtr();
    }
};

struct TemplateParamType
{
    bool isPack = false;
    const clang::TemplateTypeParmDecl* decl = nullptr;
};

struct WellKnownType : TemplateType
{
    enum Types
    {
        StdArray,
        StdString,
        StdWstring,
        StdVector,
        StdList,
        StdDeque,
        StdMap,
        StdSet,
        StdUnorderedMap,
        StdUnorderedSet,
        StdSharedPtr,
        StdUniquePtr,
        StdOptional,
        BoostOptional,
        BoostVariant
    };

    Types type = StdString;
};

struct ArrayType
{
    std::vector<uint64_t> dims;
    TypeInfoPtr itemType;
};

struct EnumType
{
    const clang::EnumDecl* decl;
};

struct DecltypeType
{
    const clang::DecltypeType* declType;
    const clang::Expr* declTypeExpr;
};

struct GenericNamedType
{
};

class TypeInfo
{
public:
    using Type = boost::variant<NoType, BuiltinType, RecordType, TemplateType, WellKnownType, ArrayType, EnumType, DecltypeType, TemplateParamType, GenericNamedType>;

    struct TypeDescr
    {
        Type type;
        std::string name;
        std::string scopeSpec;
        std::string namespaceQual;
        bool isConst = false;
        bool isVolatile = false;
        bool isReference = false;
        bool isRVReference = false;
        int pointingLevels = 0;
    };

    TypeInfo();

    const auto& GetType() const {return m_type;}
    auto getAsBuiltin() const
    {
        return boost::get<const BuiltinType>(&m_type);
    }
    auto getAsRecord() const
    {
        return boost::get<const RecordType>(&m_type);
    }
    auto getAsTemplate() const
    {
        return boost::get<const TemplateType>(&m_type);
    }
    auto getAsWellKnownType() const
    {
        return boost::get<const WellKnownType>(&m_type);
    }
    auto getAsArrayType() const
    {
        return boost::get<const ArrayType>(&m_type);
    }
    auto getAsEnumType() const
    {
        return boost::get<const EnumType>(&m_type);
    }
    auto getAsDecltypeType() const
    {
        return boost::get<const DecltypeType>(&m_type);
    }
    auto getAsTemplateParamType() const
    {
        return boost::get<const TemplateParamType>(&m_type);
    }
    auto isNoType() const
    {
        return boost::get<const NoType>(&m_type) != nullptr;
    }

    bool getIsConst() const
    {
        return m_isConst;
    }
    bool getIsVolatile() const
    {
        return m_isVolatile;
    }
    bool getIsReference() const
    {
        return m_isReference;
    }
    bool getIsRVReference() const
    {
        return m_isRVReference;
    }
    int getPointingLevels() const
    {
        return m_pointingLevels;
    }
    std::string getDeclaredName() const
    {
        return m_declaredName;
    }
    std::string getScopedName() const
    {
        return m_scopedName;
    }
    std::string getFullQualifiedName() const
    {
        return m_fullQualifiedName;
    }
    std::string getPrintedName() const
    {
        return m_printedName;
    }
    const clang::Type* getTypeDecl() const
    {
        return m_typeDecl;
    }
    bool canBeMoved() const
    {
        struct Visitor : public boost::static_visitor<bool>
        {
            Visitor(const TypeInfo* ti) : m_type(ti) {}

            bool operator() (const NoType&) const {return false;}
            bool operator() (const BuiltinType&) const {return false;}
            bool operator() (const ArrayType&) const {return false;}
            bool operator() (const EnumType&) const {return false;}
            bool operator() (const DecltypeType&) const {return false;}
            bool operator() (const TemplateParamType&) const {return true;}
            bool operator() (const GenericNamedType&) const { return false; }

            bool operator() (const RecordType&) const
            {
                if (m_type->m_isRVReference)
                    return true;

                if (m_type->m_isConst)
                    return false;

                if (m_type->m_pointingLevels != 0)
                    return false;
                // clang::CXXRecordDecl
                return true;
            }

            bool operator() (const TemplateType&) const
            {
                if (m_type->m_isRVReference)
                    return true;

                if (m_type->m_isConst || m_type->m_isReference)
                    return false;

                if (m_type->m_pointingLevels != 0)
                    return false;
                // clang::CXXRecordDecl
                return true;
            }

            const TypeInfo* m_type;
        };

        return boost::apply_visitor(Visitor(this), m_type);
    }

    TypeDescr getTypeDescr() const;

    static TypeInfoPtr Create(const clang::QualType& qt, const clang::ASTContext* astContext);
    static TypeInfoPtr Create(const TypeDescr& descr);

private:
    bool m_isConst = false;
    bool m_isVolatile = false;
    bool m_isReference = false;
    bool m_isRVReference = false;
    int m_pointingLevels = 0;
    std::string m_declaredName;
    std::string m_scopedName;
    std::string m_fullQualifiedName;
    std::string m_printedName;
    Type m_type;
    const clang::Type* m_typeDecl;

    friend class TypeUnwrapper;

    friend std::ostream& operator << (std::ostream& os, const TypeInfoPtr& tp)
    {
        os << "(" << tp->m_printedName << ")-" << tp->m_type;
        const char* suffix = " ";
        auto print = [&os, &suffix](bool flag, auto msg)
        {
            if (flag)
            {
                os << suffix << msg;
                suffix = ", ";
            }
        };

        print(tp->m_isConst, "const");
        print(tp->m_isVolatile, "volatile");
        print(tp->m_isReference, "&");
        print(tp->m_isRVReference, "&&");
        std::string pointers;
        for (int n = 0; n < tp->m_pointingLevels; ++ n)
            pointers += "*";
        print(tp->m_pointingLevels != 0, pointers);
        return os;
    }
};

inline std::ostream& operator << (std::ostream& os, const NoType& tp)
{
    os << "NOTYPE";
    return os;
}

inline std::ostream& operator << (std::ostream& os, const BuiltinType& tp)
{
    os << "BUILTIN[type=" << (int)tp.type << ", bits=" << tp.bits << ", isSigned=" << (tp.isSigned == BuiltinType::Signed ? "true" : (tp.isSigned == BuiltinType::Unsigned ? "false" : "nosign")) << ", kind=" << (int)tp.kind << "]";
    return os;
}

inline std::ostream& operator << (std::ostream& os, const RecordType& tp)
{
    os << "RECORD[" << tp.decl->getQualifiedNameAsString() << "]";
    return os;
}

inline std::ostream& operator << (std::ostream& os, const TemplateType::GenericArg& ga)
{
    os << "GENERIC(" << ga.kind << ")";
    return os;
}

inline std::ostream& operator << (std::ostream& os, const TemplateType& tp)
{
    os << "TEMPLATE[" << tp.decl->getQualifiedNameAsString() << "<";
    WriteSeq(os, tp.arguments);
    os << ">]";
    return os;
}

inline std::ostream& operator << (std::ostream& os, const WellKnownType& tp)
{
    os << "WELLKNOWN[type=" << tp.type << "]";
    return os;
}

inline std::ostream& operator << (std::ostream& os, const ArrayType& tp)
{
    os << "ARRAY[type="
       << tp.itemType
       << ", dims={";
    WriteSeq(os, tp.dims);
    os << "}]";
    return os;
}

inline std::ostream& operator << (std::ostream& os, const EnumType& tp)
{
    os << "ENUM[" << tp.decl->getQualifiedNameAsString() << "]";
    return os;
}

inline std::ostream& operator << (std::ostream& os, const DecltypeType& tp)
{
    os << "DECLTYPE[]";
    return os;
}

inline std::ostream& operator << (std::ostream& os, const TemplateParamType& tp)
{
    os << "AUTO[ispack=" << tp.isPack << "]";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const GenericNamedType& tp)
{
    os << "GENERIC[]";
    return os;
}

} // reflection

#endif // TYPE_INFO_H

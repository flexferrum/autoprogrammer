#ifndef INCLUDE_META_METACLASS_H
#define INCLUDE_META_METACLASS_H

#include <vector>

namespace meta
{
class CompilerImpl
{
public:
    void require(bool, const char* message);
    void message(const char* msg);
    void error(const char* msg);
    void warning(const char* msg);
    template<typename T>
    void dump(const T& obj);
};


template<typename T>
struct Range
{
    struct iterator
    {
        T& operator*();
        T* operator->();
        bool operator == (const iterator&) const;
        bool operator != (const iterator&) const;
        iterator& operator++();
        iterator operator++(int);
    };

    bool empty() const;
    iterator begin();
    iterator end();
    T* find(const char* name);
    const T* find(const char* name) const;
};

enum class AccessType
{
    Unspecified,
    Public,
    Private,
    Protected
};

class TypeInfo
{
public:
};

class ClassMemberBase
{
public:
    const std::string& name() const;
    bool has_access() const;
    AccessType get_access() const;

    bool is_public() const;
    bool is_protected() const;
    bool is_private() const;

    void make_public();
    void make_protected();
    void make_private();

    void rename(const char* name);
    void remove();
};

class MemberInfo : public ClassMemberBase
{
public:
    const TypeInfo& type();
};

class MethodInfo : public ClassMemberBase
{
public:
    struct ParamInfo
    {
        int position();
        const std::string& name();
        const TypeInfo& type();
        bool is_default();
    };

    const TypeInfo& return_type() const;
    const Range<ParamInfo>& params() const;
    Range<ParamInfo>& params();

    bool is_copy_ctor() const;
    bool is_move_ctor() const;
    bool is_default_ctor() const;
    bool is_ctor() const;
    bool is_dtor() const;
    bool is_copy_assign() const;
    bool is_move_assign() const;
    bool is_defaulted() const;
    bool is_deleted() const;
    bool is_implicit() const;
    bool is_operator() const;
    bool is_static() const;
    bool is_constexpr() const;
    bool is_strict_constexpr() const;
    bool is_virtual() const;
    bool is_noexcept() const;
    bool is_const() const;
    bool is_rv_ref() const;
    bool is_template() const;
    bool is_pure_virtual() const;

    void make_defaulted();
    void make_deleted();
    void make_implicit();
    void make_operator();
    void make_static();
    void make_constexpr();
    void make_strict_constexpr();
    void make_virtual();
    void make_noexcept();
    void make_const();
    void make_rv_ref();
    void make_pure_virtual();
};

class ClassInfo
{
public:
    const std::string& name() const;

    Range<MemberInfo>& variables() const;
    Range<MethodInfo>& functions() const;
};

namespace detail
{
class MetaClassBase
{
};

class MetaClassImplBase
{
};

} // detail
} // meta

#define METACLASS_DECL(ClassName) \
struct MetaClass_##ClassName : public meta::detail::MetaClassBase \
{ \
    static meta::CompilerImpl& compiler; \
    static meta::ClassInfo& $##ClassName; \
    \
    struct ClassName; \
}; \
struct ClassName##_Meta; \
struct MetaClass_##ClassName::ClassName

#define METACLASS_INST(MetaName, InstClassName) \
struct MetaClassInstance_##InstClassName : public meta::detail::MetaClassImplBase \
{ \
    using Metaclass = MetaName##_Meta; \
    class InstClassName; \
}; \
\
class MetaClassInstance_##InstClassName::InstClassName

#define META_INJECT [&]()

#endif // INCLUDE_META_METACLASS_H

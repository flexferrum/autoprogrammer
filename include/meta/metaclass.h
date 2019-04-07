#ifndef INCLUDE_META_METACLASS_H
#define INCLUDE_META_METACLASS_H

#include <vector>
#include <string>

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
    template<typename T>
    TypeInfo& operator = (T&& other);
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
    TypeInfo type() const;
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

    TypeInfo return_type() const;
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

    template<typename ... Args>
    TypeInfo operator()(Args&& ... args) const;
};

class ClassInfo
{
public:
    const std::string& name() const;

    Range<MemberInfo>& variables() const;
    Range<MethodInfo>& functions() const;
    template<typename T>
    void add(T entity, AccessType access = AccessType::Unspecified);
};

namespace detail
{
class MetaClassBase
{
};

class MetaClassImplBase
{
};

template<typename T>
struct TypeProjector
{
    using type = T;
};

} // detail

using MetaclassMethodPtr = void (*)(ClassInfo dst, const ClassInfo& src);
extern CompilerImpl compiler;

template<typename T>
T& project(T&& val);

template<typename T>
detail::TypeProjector<T> project_type(T&&);

TypeInfo template_type(std::string name);

template<typename T>
TypeInfo& reflect_type();

template<typename ... T>
Range<TypeInfo>& reflect_type();
} // meta

#define METACLASS_INST_IMPL(InstClassName, ClassType, ...) \
struct MetaClassInstance_##InstClassName : public meta::detail::MetaClassImplBase \
{ \
    static constexpr std::initializer_list<meta::MetaclassMethodPtr> metaPtrList_ = {__VA_ARGS__}; \
    class InstClassName; \
}; \
\
ClassType MetaClassInstance_##InstClassName::InstClassName

#define METACLASS_INST(InstClassName, MetaName) METACLASS_INST_IMPL(InstClassName, class, MetaName)

#ifdef FL_CODEGEN_INVOKED_
#define META_CONSTEXPR [[gsl::suppress("constexpr")]]
#define META_INJECT(target, vis) [[gsl::suppress("inject", "$target=" #target, #vis)]]
#define META_ENUM()
#else
#define META_CONSTEXPR
#define META_INJECT(target, vis)
#define META_ENUM()
#endif

#define $_metaclass(N) METACLASS_DECL(N)
#define $_class(N1, ...) METACLASS_INST_IMPL(N1, class, __VA_ARGS__)
#define $_struct(N1, ...) METACLASS_INST_IMPL(N1, struct, __VA_ARGS__)
#define $_inject_v(T, V) META_INJECT(T, V)
#define $_inject(T) META_INJECT(T, "")
#define $_constexpr META_CONSTEXPR
#define $_t(v) decltype(meta::project_type(v))
#define $_v(v) meta::project(v)
#define t_$(v) meta::reflect_type<v>()
#define $_str(str) meta::project(#str)

#endif // INCLUDE_META_METACLASS_H

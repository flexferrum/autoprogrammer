#ifndef INCLUDE_META_METACLASS_H
#define INCLUDE_META_METACLASS_H

#include <vector>

namespace meta
{
class CompilerImpl
{
public:
    void require(bool, const char* message);
};

class VariableInfo
{
public:
};

class MethodInfo
{
public:
    bool is_copy() const;
    bool is_move() const;
    bool has_access() const;
    void make_public();
    bool is_public() const;
    void make_pure_virtual();
};

template<typename T>
struct Range
{
    bool empty() const;
    T* begin();
    T* end();
};

class ClassInfo
{
public:
    Range<VariableInfo>& variables() const;
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

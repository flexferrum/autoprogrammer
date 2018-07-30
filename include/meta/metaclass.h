#ifndef INCLUDE_META_METACLASS_H
#define INCLUDE_META_METACLASS_H

namespace meta
{
class CompilerImpl;

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
        \
        struct ClassName; \
}; \
\
struct MetaClass_##ClassName::ClassName

#define METACLASS_INST(MetaName, InstClassName) \
struct MetaClassInstance_##MetaName : public meta::detail::MetaClassImplBase \
{ \
        class InstClassName; \
}; \
\
class MetaClassInstance_##MetaName::InstClassName

#define META_INJECT [&]()

#endif // INCLUDE_META_METACLASS_H

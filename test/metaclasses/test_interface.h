#ifndef TEST_ENUMS_H
#define TEST_ENUMS_H

#include <meta/metaclass.h>

#include <string>
#include <utility>

// 'Interface' metaclass declaration
template<typename V>
inline void Visitable(meta::ClassInfo dst, const meta::ClassInfo& src)
{
    using meta::compiler;

//    compiler.message("Hello world from metaprogrammer! (1)");

    for (auto& f : src.functions())
        $_inject_v(dst, public) f;

    // $_inject(dst) [name="Visit", is_const=true](const V* v) -> void {v->Visit($_str(*this));};
    $_inject_v(dst, public) { void Visit(const Visitor*); }
}

// Declare template metaclass for visitors across 'Types' list
template<typename R, typename ... Types>
inline void Visitor(meta::ClassInfo dst, const meta::ClassInfo& src)
{
    // Insert all methods form src to dst
    for (auto& f : src.functions())
        $_inject_v(dst, public) f;

    // Add extra 'Visit' methods to dst dependent on specific type from 'Types'
    for (auto& t : t_$(Types ...))
        $_inject_v(dst, public) { R Visit(const $_t(t)& obj); }
}

template<typename ... Types>
inline void CRTPVisitor(meta::ClassInfo dst, const meta::ClassInfo& src)
{
    // Add template parameter to the metaclass instance declaration
    const auto& tplParam = dst.add_template_type_param("Derived");

    // Insert all methods form src to dst
    for (auto& f : src.functions())
        $_inject_v(dst, public) f;

    // Inject generic function call operator
    $_inject_v(dst, public) [name="operator()"](auto&& obj) -> void {
        $_v("GetDerived")()->$_mem("Visit")(std::forward<$_str(T0)>(obj));
    };

    // Add extra fallback 'Visit' methods to dst dependent on specific type from 'Types'
    for (auto& t : t_$(Types ...))
        $_inject_v(dst, public) [name="Visit"](const $_t(t)& obj) -> void
        {
        };

    // Add private 'GetDerived' method which performs the simple static_cast
    $_inject_v(dst, private) [name="GetDerived"]() -> $_t(tplParam)* {
        return static_cast<$_t(tplParam)*>($_str(this));
    };
}

inline void Interface(meta::ClassInfo dst, const meta::ClassInfo& src)
{
    using meta::compiler;

    compiler.message("Hello world from metaprogrammer! (2)");
    compiler.require(src.variables().empty(), "Interface may not contain data members");

    META_INJECT(dst, public) {
        enum {InterfaceId = 123456};
    }

    for (auto& f : src.functions())
    {
        compiler.require(f.is_implicit() || (!f.is_copy_ctor() && !f.is_move_ctor()),
            "Interface can't contain copy or move constructor");

        if (!f.is_implicit())
        {
            f.make_public();

            compiler.require(f.is_public(), "Inteface function must be public");

            f.make_pure_virtual();

            $_inject(dst) f;
        }
    }
}

inline void BoostSerializable(meta::ClassInfo dst, const meta::ClassInfo& src)
{
    for (auto& v : src.variables())
        $_inject_v(dst, public) v;

    $_inject_v(dst, public) [&, name="serialize"](auto& ar, unsigned int ver) -> void
    {
        $_constexpr for (auto& v : src.variables())
            $_inject(_) ar & $_v(v.name());
    };
};

class Visitor
{
public:
};

struct A
{
};

struct B
{
};

struct C
{
};

struct D
{
};

$_class(SomeVisitor, CRTPVisitor<A, B, C, D>) // , Interface)
{
public:
    void TestMethod1();
    std::string TestMethod2(int param) const;
};

$_class(TestStruct, BoostSerializable)
{
public:
    int a;
    float floatVal;
    std::string b;
    std::string helloWorld23;
};

#endif // TEST_ENUMS_H

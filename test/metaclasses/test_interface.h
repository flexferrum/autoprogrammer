#ifndef TEST_ENUMS_H
#define TEST_ENUMS_H

#include <meta/metaclass.h>

#include <string>
#include <utility>

// 'Interface' metaclass declaration
template<typename V>
void Visitable(meta::ClassInfo dst, const meta::ClassInfo& src)
{
    using meta::compiler;

//    compiler.message("Hello world from metaprogrammer! (1)");

    for (auto& f : src.functions())
        $_inject_v(dst, public) f;

    // $_inject(dst) [name="Visit", is_const=true](const V* v) -> void {v->Visit($_str(*this));};
    $_inject_v(dst, public) { void Visit(const Visitor*); }
}

// Declare template metaclass for visitors across 'Types' list
template<typename ... Types>
inline void Visitor(meta::ClassInfo dst, const meta::ClassInfo& src)
{
    // Insert all methods form src to dst
    for (auto& f : src.functions())
        $_inject_v(dst, public) f;

    // Add extra 'Visit' methods to dst dependent on specific type from 'Types'
    for (auto& t : t_$(Types ...))
        $_inject_v(dst, public) { void Visit(const $_t(t)& obj); }
}

void Interface(meta::ClassInfo dst, const meta::ClassInfo& src)
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

$_class(SomeVisitor, Visitor<A, B, C>, Interface)
{
public:
    void TestMethod1();
    std::string TestMethod2(int param) const;
};

#endif // TEST_ENUMS_H

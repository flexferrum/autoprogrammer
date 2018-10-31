#ifndef TEST_ENUMS_H
#define TEST_ENUMS_H

#include <meta/metaclass.h>

#include <string>
#include <utility>

// 'Interface' metaclass declaration
METACLASS_DECL(Interface)
{
    static void GenerateDecl()
    {
        compiler.message("Hello world from metaprogrammer!");
        compiler.require($Interface.variables().empty(), "Interface may not contain data members");

        META_INJECT(public) {
            enum {InterfaceId = 123456};
        }

        for (auto& f : $Interface.functions())
        {
            compiler.require(f.is_implicit() || (!f.is_copy_ctor() && !f.is_move_ctor()),
                "Interface can't contain copy or move constructor");

            if (!f.is_implicit())
            {
                f.make_public();

                compiler.require(f.is_public(), "Inteface function must be public");

                f.make_pure_virtual();
#if 0
                META_INJECT(public) [name=f.name(), is_virtual=true]($_t(f) fn, $_t(f.return_type())& result) -> int
                {
                    try
                    {
                        result = fn(fn.params());
                        return 0;
                    }
                    catch (...)
                    {
                        return 1;
                    }
                };
#endif
            }
        }
    }
};

METACLASS_INST(TestIface, Interface)
{
    void TestMethod1();
    std::string TestMethod2(int param) const;
};

#if 0
METACLASS_INST(Interface, BadTestIface)
{
public:
    BadTestIface(const BadTestIface&);

    void TestMethod1();
    std::string TestMethod2(int param) const;

protected:
    void TestMethod3();

private:
    int m_val;
};
#endif

#endif // TEST_ENUMS_H

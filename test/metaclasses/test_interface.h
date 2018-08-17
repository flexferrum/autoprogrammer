#ifndef TEST_ENUMS_H
#define TEST_ENUMS_H

#include <meta/metaclass.h>

#include <string>
#include <utility>

METACLASS_DECL(Interface)
{
    static void GenerateDecl()
    {
        compiler.require($Interface.variables().empty(), "Interface may not contain data members");

        for (auto& f : $Interface.functions())
        {
            compiler.require(!f.is_copy() && !f.is_move(), "Interface can't contain copy or move constructor");
            if (!f.has_access())
                f.make_public();
                
            compiler.require(f.is_public(), "Inteface function must be public");
            f.make_pure_virtual();
        }
    }
    
    constexpr static int GenerateImpl()
    {
        int a = 0;
        return a + 2;
    }
};

METACLASS_INST(Interface, TestIface)
{
public:
    void TestMethod1();
    std::string TestMethod2(int param) const;
};

#endif // TEST_ENUMS_H

#ifndef TEST_ENUMS_H
#define TEST_ENUMS_H

#include <meta/metaclass.h>

#include <string>
#include <utility>

METACLASS_DECL(Interface)
{
};

METACLASS_INST(Interface, TestIface)
{
public:
    void TestMethod1();
    std::string TestMethod2(int param) const;
};

#endif // TEST_ENUMS_H

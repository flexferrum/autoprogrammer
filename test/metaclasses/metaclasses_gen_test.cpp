#include <gtest/gtest.h>
#include <flex_lib/detector.h>

#include "test_interface.h"
#include <generated/test_interface.meta.h>
//#include <generated/boost_serialization.meta.h>

#include <array>
#include <iterator>

class MyVisitor : public SomeVisitor<MyVisitor>
{
public:
    void Visit(const A &obj)
    {
        std::cout << "A visited" << std::endl;
    }
    ;
};


int main()
{
    A a;
    MyVisitor visitor;

    visitor(a);

    return 0;
}

#if 0
class TestIfaceImpl : public TestIface
{
    // TestIface interface
public:
    void TestMethod1() override;
    std::string TestMethod2(int param) const override;
};

void TestIfaceImpl::TestMethod1()
{
}

std::string TestIfaceImpl::TestMethod2(int) const
{
    return "Hello World!";
}

#endif

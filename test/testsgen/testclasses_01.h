#ifndef TESTCLASSES_01_H
#define TESTCLASSES_01_H

class TestClass01
{
public:
    class TestHelper;
    TestClass01();
    TestClass01(int someValue);

//    TestClass01& operator = (const TestClass01& other)
//    {
//        return *this;
//    }

    bool operator < (const TestClass01& other) const
    {
        return m_someValue < other.m_someValue;
    }

    int operator [](int num) const
    {
        return m_someValue * num;
    }

    int operator ()(int num) const
    {
        return m_someValue + num;
    }

    auto Square() const
    {
        return m_someValue * m_someValue;
    }

private:
    int m_someValue;
};

#endif // TESTCLASSES_01_H

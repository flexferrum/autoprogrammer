#ifndef TEST_PIMPL_IMPL_H
#define TEST_PIMPL_IMPL_H

#include "test_pimpl.h"

#include <string>
#include <array>

class TestPimpl;

class TestPimplImpl
{
public:
    TestPimplImpl(TestPimpl*, uint32_t number)
        : m_intNumber(number)
    {        
    }

    TestPimplImpl(TestPimpl*, std::string number)
        : m_strNumber(number)
    {
    }
    
    TestPimplImpl(TestPimpl*, TestMoveable&& moveable)
        : m_moveableObj(std::move(moveable))
    {
    }
    
    const std::string GetString() const
    {
        return m_strNumber;
    }
    const uint32_t GetNumber() const
    {
        return m_intNumber;
    }
    void SetGeneratedValues(unsigned values[10])
    {
        std::copy(values, values + 10, begin(m_values));
    }
    const std::array<unsigned, 10> GetGeneratedValues() const
    {
        return m_values;
    }
    void ResetValues(int num, std::string str)
    {
        m_intNumber = num;
        m_strNumber = str;
    }

    PimplMode GetCurrentMode() const
    {
        return m_curMode;
    }
    const int* GetMoveablePtr() const
    {
        return m_moveableObj.GetObject();
    }
    
    bool operator == (const TestPimplImpl& pimpl) const
    {
        return m_intNumber == pimpl.m_intNumber;
    }
    
private:
    uint32_t m_intNumber;
    std::string m_strNumber;
    std::array<unsigned, 10> m_values;
    PimplMode m_curMode = AbnormalMode;
    TestMoveable m_moveableObj = TestMoveable(5);
};

#endif // TEST_PIMPL_IMPL_H

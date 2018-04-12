#ifndef TEST_ENUMS_H
#define TEST_ENUMS_H

#include <flex_lib/pimpl.h>

#include <string>
#include <utility>

class TestPimplImpl;

enum PimplMode
{
    NormalMode,
    AbnormalMode
};

class TestMoveable
{
public:
    TestMoveable(int value = 10)
        : m_object(new int(value))
    {
    }
    
    const int* GetObject() const {return m_object.get();}
    
private:
    std::unique_ptr<int> m_object;
};

class TestPimpl : flex_lib::pimpl<TestPimplImpl>
{
public:
    explicit TestPimpl(uint32_t number = 0);
    explicit TestPimpl(std::string number);
    explicit TestPimpl(TestMoveable&& obj) noexcept;
    ~TestPimpl() noexcept;
    
    const std::string GetString() const;
    const uint32_t GetNumber() const;
    void SetGeneratedValues(unsigned values[10]);
    const std::array<unsigned, 10> GetGeneratedValues() const;
    void ResetValues(int num, std::string str);
    PimplMode GetCurrentMode() const;
    const int* GetMoveablePtr() const noexcept;
    
    bool operator == (const TestPimpl& other) const;
};

#endif // TEST_ENUMS_H

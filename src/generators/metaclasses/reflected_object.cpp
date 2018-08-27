#include "reflected_object.h"
#include "value.h"

#include <iostream>

namespace codegen
{
namespace interpreter
{

template<typename Coll>
class StdCollectionRefRange : public RangeT
{
public:
    StdCollectionRefRange(Coll* coll)
        : m_coll(coll)
    {}

    // RangeT interface
    bool Empty() override
    {
        return m_coll->empty();
    }

private:
    Coll* m_coll;
};

template<typename Coll>
auto MakeStdCollectionRefRange(Coll* coll)
{
    return std::make_shared<StdCollectionRefRange<Coll>>(coll);
}


ReflectedObject::ReflectedObject(DataType val)
    : m_value(std::move(val))
{

}

bool ReflectedMethods::Compiler_message(InterpreterImpl* interpreter, const Compiler& obj, Value& result, const std::string& msg)
{
    std::cout << "###### " << msg << std::endl;
    result = VoidValue();
    return true;
}

bool ReflectedMethods::Compiler_require(InterpreterImpl* interpreter, const Compiler& obj, Value& result, bool testResult, const std::string& msg)
{
    result = VoidValue();
    if (!testResult)
        std::cout << "###### " << msg << std::endl;

    return true;
}

bool ReflectedMethods::ClassInfo_variables(InterpreterImpl* interpreter, reflection::ClassInfoPtr obj, Value& result)
{
    auto rangePtr = MakeStdCollectionRefRange(&obj->members);
    result = Value(ReflectedObject(rangePtr));

    std::cout << "#### Range to ClassInfo::variables created" << std::endl;
    return true;
}

bool ReflectedMethods::ClassInfo_functions(InterpreterImpl* interpreter, reflection::ClassInfoPtr obj, Value& result)
{
    auto rangePtr = MakeStdCollectionRefRange(&obj->methods);
    result = Value(ReflectedObject(rangePtr));

    std::cout << "#### Range to ClassInfo::functions created" << std::endl;
    return true;
}

bool ReflectedMethods::RangeT_empty(InterpreterImpl* interpreter, RangeTPtr obj, Value& result)
{
    result = Value(obj->Empty());

    std::cout << "#### ReflectedMethods::RangeT_empty called" << std::endl;
    return true;
}

} // interpreter
} // codegen

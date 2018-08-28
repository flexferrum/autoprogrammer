#include "reflected_object.h"
#include "value.h"

#include <iostream>

namespace codegen
{
namespace interpreter
{
    
template<typename It>
class StdCollectionIterator : public IteratorT
{
public:
    StdCollectionIterator(It it)
        : m_it(it)
    {}
    
private:
    It m_it;
};

template<typename It>
auto MakeStdCollectionIterator(It&& it)
{
    return std::make_shared<StdCollectionIterator<It>>(std::forward<It>(it));
}

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
        std::cout << "StdCollectionRefRange::Empty. Coll=" << m_coll << std::endl;
        return m_coll->empty();
    }

    Value Begin() override
    {
        std::cout << "StdCollectionRefRange::Begin. Coll=" << m_coll << std::endl;
        return ReflectedObject(MakeStdCollectionIterator(m_coll->begin()));
    }
    Value End() override
    {
        std::cout << "StdCollectionRefRange::End. Coll=" << m_coll << std::endl;
        return ReflectedObject(MakeStdCollectionIterator(m_coll->end()));
    }

    Value ConstBegin() override {return Value();}
    Value ConstEnd() override {return Value();}
    
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
    std::cout << "#### ReflectedMethods::RangeT_empty called. Object: " << obj << std::endl;
    result = Value(obj->Empty());

    return true;
}

bool ReflectedMethods::RangeT_begin(InterpreterImpl* interpreter, RangeTPtr obj, Value& result)
{
    result = Value(obj->Begin());

    std::cout << "#### ReflectedMethods::RangeT_begin called. Object: " << obj << std::endl;
    return true;
}

bool ReflectedMethods::RangeT_end(InterpreterImpl* interpreter, RangeTPtr obj, Value& result)
{
    result = Value(obj->End());

    std::cout << "#### ReflectedMethods::RangeT_end called. Object: " << obj << std::endl;
    return true;
}
} // interpreter
} // codegen

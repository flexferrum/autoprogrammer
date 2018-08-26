#ifndef REFLECTED_OBJECT_H
#define REFLECTED_OBJECT_H

#include "decls_reflection.h"

#include <boost/variant.hpp>

namespace codegen
{
namespace interpreter
{

class InterpreterImpl;
class Value;

struct Compiler
{
};

class RangeT
{
public:
    virtual ~RangeT() {}

    virtual bool Empty() = 0;
};

using RangeTPtr = std::shared_ptr<RangeT>;

class ReflectedObject
{
public:
    using DataType = boost::variant<Compiler, reflection::ClassInfoPtr, reflection::MethodInfoPtr, reflection::MemberInfoPtr, RangeTPtr>;

    ReflectedObject(DataType = DataType());
    template<typename U>
    ReflectedObject(U&& val)
        : m_value(std::move(val))
    {}

    auto& GetValue() {return m_value;}
    auto& GetValue() const {return m_value;}

private:
    DataType m_value;
};

class ReflectedMethods
{
public:
    static bool Compiler_message(InterpreterImpl* interpreter, const Compiler& obj, Value& result, const std::string& msg);
    static bool Compiler_require(InterpreterImpl* interpreter, const Compiler& obj, Value& result, bool testResult, const std::string& msg);
    static bool ClassInfo_variables(InterpreterImpl* interpreter, reflection::ClassInfoPtr obj, Value& result);
    static bool RangeT_empty(InterpreterImpl* interpreter, RangeTPtr obj, Value& result);
};

} // interpreter
} // codegen

#endif // REFLECTED_OBJECT_H

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
    virtual Value Begin() = 0;
    virtual Value End() = 0;
    virtual Value ConstBegin() = 0;
    virtual Value ConstEnd() = 0;
};

using RangeTPtr = std::shared_ptr<RangeT>;

class IteratorT
{
public:
    virtual ~IteratorT() {}

    virtual bool IsEqual(const IteratorT* other) const = 0;
    virtual Value GetValue() const = 0;
    virtual void PrefixInc() = 0;
    // virtual Value PostfixInc() const = 0;
};

using IteratorTPtr = std::shared_ptr<IteratorT>;

class ReflectedObject
{
public:
    using DataType = boost::variant<Compiler, reflection::ClassInfoPtr, reflection::MethodInfoPtr, reflection::MemberInfoPtr, RangeTPtr, IteratorTPtr>;

    ReflectedObject(DataType = DataType());

    ReflectedObject(const ReflectedObject& val) = default;
    ReflectedObject(ReflectedObject&& val) = default;
#ifndef _MSC_VER
    ReflectedObject(ReflectedObject& val)
        : ReflectedObject(std::move(val))
    {}
#endif

    template<typename U>
    ReflectedObject(U&& val)
        : m_value(std::move(val))
    {}

    ReflectedObject& operator = (const ReflectedObject& val) = default;
    ReflectedObject& operator = (ReflectedObject&&) = default;

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
    static bool ClassInfo_functions(InterpreterImpl* interpreter, reflection::ClassInfoPtr obj, Value& result);

    static bool MethodInfo_is_public(InterpreterImpl* interpreter, reflection::MethodInfoPtr obj, Value& result);
    static bool ClassMemberBase_has_access(InterpreterImpl* interpreter, reflection::MethodInfoPtr obj, Value& result);
    static bool ClassMemberBase_make_public(InterpreterImpl* interpreter, reflection::MethodInfoPtr obj, Value& result);

    static bool MethodInfo_is_implicit(InterpreterImpl* interpreter, reflection::MethodInfoPtr obj, Value& result);
    static bool MethodInfo_is_copy_ctor(InterpreterImpl* interpreter, reflection::MethodInfoPtr obj, Value& result);
    static bool MethodInfo_is_move_ctor(InterpreterImpl* interpreter, reflection::MethodInfoPtr obj, Value& result);
    static bool MethodInfo_make_pure_virtual(InterpreterImpl* interpreter, reflection::MethodInfoPtr obj, Value& result);

    static bool RangeT_empty(InterpreterImpl* interpreter, RangeTPtr obj, Value& result);
    static bool RangeT_begin(InterpreterImpl* interpreter, RangeTPtr obj, Value& result);
    static bool RangeT_end(InterpreterImpl* interpreter, RangeTPtr obj, Value& result);

    static bool IteratorT_OperNotEqual_Same(InterpreterImpl* interpreter, IteratorTPtr left, Value& result, IteratorTPtr right);
    static bool IteratorT_OperStar(InterpreterImpl* interpreter, IteratorTPtr left, Value& result);
    static bool IteratorT_OperPrefixInc(InterpreterImpl* interpreter, IteratorTPtr left, Value& result);
};

} // interpreter
} // codegen

#endif // REFLECTED_OBJECT_H

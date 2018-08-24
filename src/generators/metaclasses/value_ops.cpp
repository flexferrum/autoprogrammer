#include "value_ops.h"
#include "cpp_interpreter_impl.h"

#include <iostream>

namespace codegen
{
namespace interpreter
{

namespace value_ops
{
Value* GetActualValue(Value& val)
{
    Value::Ptr* ptr;
    if (!(ptr = boost::get<Value::InternalRef>(&val.GetValue())))
    {
        if ((ptr = boost::get<Value::Reference>(&val.GetValue())))
        {
            ptr = boost::get<Value::Pointer>(&val.GetValue());
        }
    }

    return ptr == nullptr ? &val : ptr->pointee;
}


template<typename Fn, typename Value>
auto ApplyUnwrapped(Value&& val, Fn&& fn)
{
    auto actualVal = GetActualValue(val);

    ReflectedObject* reflObj = boost::get<ReflectedObject>(&actualVal->GetValue());

    if (reflObj != nullptr)
        return fn(reflObj->GetValue());
//    else if (targetString != nullptr)
//        return fn(*actualVal);
//    else if (internalValueRef != nullptr)
//        return fn(internalValueRef->get());

    return fn(actualVal->GetValue());
}

template<typename V, typename Value, typename ... Args>
auto Apply(Value&& val, Args&& ... args)
{
    return ApplyUnwrapped(val, [&args...](auto& val) {
        return boost::apply_visitor(V(args...), val);
    });
}

template<typename V, typename Value, typename ... Args>
auto Apply2(const Value&& val1, const Value&& val2, Args&& ... args)
{
    return ApplyUnwrapped(std::forward<Value>(val1), [&val2, &args...](auto& uwVal1) {
        return ApplyUnwrapped(std::forward<Value>(val2), [&uwVal1, &args...](auto& uwVal2) {
            return boost::apply_visitor(V(args...), uwVal1, uwVal2);
        });
    });
}

struct CallMemberVisitor : boost::static_visitor<bool>
{
    InterpreterImpl* interpreter;
    const clang::CXXMethodDecl* method;
    const std::vector<Value>& args;
    Value& result;
    CallMemberVisitor(InterpreterImpl* i, const clang::CXXMethodDecl* m, const std::vector<Value>& a, Value& r)
        : interpreter(i)
        , method(m)
        , args(a)
        , result(r)
    {
    }

    template<typename U>
    bool operator()(U&& val) const
    {
        std::cout << "Call method '" << method->getQualifiedNameAsString() << "' for some object." << std::endl;
        return true;
    }
};

bool CallMember(InterpreterImpl* interpreter, Value& obj, const clang::CXXMethodDecl* method, const std::vector<Value>& args, Value& result)
{
    return Apply<CallMemberVisitor>(obj, interpreter, method, args, result);
}
} // namespace value_ops
} // interpreter
} // codegen

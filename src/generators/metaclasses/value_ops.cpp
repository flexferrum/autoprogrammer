#include "value_ops.h"
#include "cpp_interpreter_impl.h"
#include "reflected_object.h"

#include <functional>
#include <utility>
#include <iostream>

using namespace std::string_literals;

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

const Value* GetActualValue(const Value& val)
{
    const Value::Ptr* ptr;
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

    auto reflObj = boost::get<ReflectedObject>(&actualVal->GetValue());

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
    return ApplyUnwrapped(std::forward<Value>(val), [visitor = V(std::forward<Args>(args)...)](auto& v) {
        return boost::apply_visitor(visitor, v);
    });
}

template<typename V, typename Value, typename ... Args>
auto Apply2(Value&& val1, Value&& val2, Args&& ... args)
{
    return ApplyUnwrapped(std::forward<Value>(val1), [&val2, &args...](auto& uwVal1) {
        return ApplyUnwrapped(std::forward<Value>(val2), [&uwVal1, visitor = V(std::forward<Args>(args)...)](auto& uwVal2) {
            return boost::apply_visitor(visitor, uwVal1, uwVal2);
        });
    });
}

template<typename R>
struct ValueConverter : boost::static_visitor<R>
{
    template<typename U>
    R operator()(U&& val, std::enable_if_t<std::is_convertible<U, R>::value>* = nullptr) const
    {
        return std::forward<U>(val);
    }

    template<typename U>
    R operator()(U&& val, std::enable_if_t<!std::is_convertible<U, R>::value>* = nullptr) const
    {
        return R();
    }
};

template<typename R, typename V>
R ConvertValue(V&& value)
{
    return Apply<ValueConverter<R>>(std::forward<V>(value));
}

bool ConvertToBool(InterpreterImpl* interpreter, const Value& obj)
{
    return ConvertValue<bool>(obj);
}


template<typename Fn>
struct CallMemberVisitor : boost::static_visitor<bool>
{
    Fn fn;
    InterpreterImpl* interpreter;
    const std::vector<Value>& args;
    Value& result;

    CallMemberVisitor(Fn f, InterpreterImpl* i, const std::vector<Value>& a, Value& r)
        : fn(f)
        , interpreter(i)
        , args(a)
        , result(r)
    {
    }

    template<typename U, typename T, typename ... Args, size_t ... Idxs>
    bool Invoke(bool (*fn)(InterpreterImpl*, T, Value&, Args...), U&& val, const std::index_sequence<Idxs...>&) const
    {
        return fn(interpreter, std::forward<U>(val), result, ConvertValue<std::decay_t<Args>>(args[Idxs])...);
    }

    template<typename U, typename T, typename ... Args>
    auto Call(bool (*fn)(InterpreterImpl*, T, Value&, Args...), U&& val) const -> decltype(fn(interpreter, std::forward<U>(val), result, std::declval<Args>()...))
    {
        return Invoke(fn, std::forward<U>(val), std::make_index_sequence<sizeof ... (Args)>());
    }

    template<typename U>
    auto operator()(U&& val) const -> decltype(Call(fn, std::forward<U>(val)))
    {
        return Call(fn, std::forward<U>(val));
    }

    auto operator()(...) const
    {
        std::cout << "Call method for some object of wrong type." << std::endl;
        return true;
    }

};

template<typename Fn>
auto ApplyCallMemberVisitor(Fn&& fnPtr, Value& obj, InterpreterImpl* interpreter, const std::vector<Value>& args, Value& result)
{
    return Apply<CallMemberVisitor<Fn>>(obj, fnPtr, interpreter, args, result);
}

bool CallMember(InterpreterImpl* interpreter, Value& obj, const clang::CXXMethodDecl* method, const std::vector<Value>& args, Value& result)
{
    static auto thunk = [](auto fnPtr, InterpreterImpl* interpreter, Value& obj, const std::vector<Value>& args, Value& result) {
        return ApplyCallMemberVisitor(fnPtr, obj, interpreter, args, result);
    };
    using ThunkType = decltype(thunk);
    using ThunkInvoker = std::function<bool (const ThunkType&, InterpreterImpl*, Value&, const std::vector<Value>&, Value&)>;

    static auto thunkMaker = [](auto fnPtr) -> ThunkInvoker {
        return [fnPtr](const ThunkType& thunk, InterpreterImpl* interpreter, Value& obj, const std::vector<Value>& args, Value& result) -> bool {
            return thunk(fnPtr, interpreter, obj, args, result);
        };
    };

    static std::unordered_map<std::string, ThunkInvoker> fns = {
        {"meta::CompilerImpl::message/void message(const char *msg)"s, thunkMaker(&ReflectedMethods::Compiler_message)},
        {"meta::CompilerImpl::require/void require(bool, const char *message)"s, thunkMaker(&ReflectedMethods::Compiler_require)},
        {"meta::ClassInfo::variables/Range<meta::MemberInfo> &variables() const"s, thunkMaker(&ReflectedMethods::ClassInfo_variables)},
        {"meta::ClassInfo::functions/Range<meta::MethodInfo> &functions() const"s, thunkMaker(&ReflectedMethods::ClassInfo_functions)},
        {"meta::Range<meta::MemberInfo>::empty/bool empty() const"s, thunkMaker(&ReflectedMethods::RangeT_empty)},
        {"meta::Range<meta::MethodInfo>::begin/meta::Range<meta::MethodInfo>::iterator begin()"s, thunkMaker(&ReflectedMethods::RangeT_begin)},
        {"meta::Range<meta::MethodInfo>::end/meta::Range<meta::MethodInfo>::iterator end()"s, thunkMaker(&ReflectedMethods::RangeT_end)},
    };

    std::string methodName;
    {
        llvm::raw_string_ostream methodNameOs(methodName);
        clang::PrintingPolicy pp = interpreter->GetDefaultPrintingPolicy();
        pp.SuppressScope = false;
        methodNameOs << method->getQualifiedNameAsString() << "/";
        method->print(methodNameOs, pp);
    }

    // auto methodName = method->getQualifiedNameAsString();
    auto p = fns.find(methodName);
    if (p == fns.end())
    {
        std::cout << "### Can't find implementation for method '" << methodName << "'" << std::endl;
        return false;
    }


    std::cout << "### Trying to call '" << methodName << "' for object of type " << obj.GetValue().which() << std::endl;
    return p->second(thunk, interpreter, obj, args, result);
}
} // namespace value_ops
} // interpreter
} // codegen

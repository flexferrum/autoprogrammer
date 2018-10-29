#ifndef VALUE_H
#define VALUE_H

#include <clang/AST/APValue.h>
#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/APSInt.h>

#include <boost/variant.hpp>

#include "reflected_object.h"

namespace codegen
{
namespace interpreter
{

struct EmptyValue {};
struct VoidValue {};

class Value
{
public:
    struct Ptr
    {
        Ptr() = default;
        explicit Ptr(Value* val)
            : pointee(val)
        {}

        Value* pointee = nullptr;
    };

    struct InternalRef : public Ptr
    {
        using Ptr::Ptr;
    };

    struct Reference : public Ptr
    {
        using Ptr::Ptr;
    };

    struct Pointer : public Ptr
    {
        using Ptr::Ptr;
    };

    using DataType = boost::variant<
        EmptyValue,
        VoidValue,
        bool,
        llvm::APInt,
        llvm::APFloat,
        std::string,
        ReflectedObject,
        InternalRef,
        Reference,
        Pointer
    >;

    Value(DataType val = DataType())
        : m_value(std::move(val))
    {}

    Value(const Value& val) = default;
    Value(Value&& val) = default;
#ifndef _MSC_VER
    Value(Value& val)
        : Value(std::move(val))
    {}
#endif

    template<typename U>
    Value(U&& val)
        : m_value(std::move(val))
    {}

    template<typename U>
    Value(const U& val)
        : m_value(std::move(val))
    {}

    Value& operator = (const Value& val) = default;
    Value& operator = (Value&&) = default;

    auto& GetValue() const {return m_value;}
    auto& GetValue() {return m_value;}

    bool IsEmpty() const
    {
        return m_value.which() == 0;
    }

    bool IsVoid() const
    {
        return m_value.which() == 1;
    }

    bool IsNoValue() const
    {
        return IsEmpty() || IsVoid();
    }

    void Clear()
    {
        m_value = EmptyValue();
    }

    void AssignValue(Value val)
    {
        InternalRef* intRef = boost::get<InternalRef>(&m_value);
        if (intRef)
            // intRef->pointee->AssignValue(std::move(val));
            intRef->pointee->AssignValue(std::move(val));
        else
            m_value = val.m_value;
    }

    std::string ToString() const;

private:
    DataType m_value;
};

namespace detail
{
struct ValueToString : public boost::static_visitor<std::string>
{
    std::string operator()(EmptyValue) const
    {
        return "";
    }
    std::string operator()(VoidValue) const
    {
        return "";
    }
    std::string operator()(bool val) const
    {
        return val ? "true" : "false";
    }
    std::string operator()(const llvm::APInt& val) const
    {
        return val.toString(10, true);
    }
    std::string operator()(const llvm::APFloat& val) const
    {
        return "TODO: float_num";
    }
    std::string operator()(std::string val) const
    {
        return std::move(val);
    }
    std::string operator()(ReflectedObject) const
    {
        return "<ReflectedObject>";
    }
    std::string operator()(const Value::InternalRef& ref) const
    {
        return ref.pointee->ToString();
    }
    std::string operator()(const Value::Reference& ref) const
    {
        return ref.pointee->ToString();
    }
    std::string operator()(const Value::Pointer& ptr) const
    {
        return ptr.pointee ? ptr.pointee->ToString() : std::string("nullptr");
    }
};
} // detail

inline std::string Value::ToString() const
{
    return boost::apply_visitor(detail::ValueToString(), m_value);
}

} // interpreter
} // codegen


#endif // VALUE_H

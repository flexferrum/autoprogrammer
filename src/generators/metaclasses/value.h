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

private:
    DataType m_value;
};

} // interpreter
} // codegen


#endif // VALUE_H

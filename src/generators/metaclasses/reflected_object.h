#ifndef REFLECTED_OBJECT_H
#define REFLECTED_OBJECT_H

#include "decls_reflection.h"

#include <boost/variant.hpp>

namespace codegen
{
namespace interpreter
{

struct Compiler
{
};

class ReflectedObject
{
public:
    using DataType = boost::variant<Compiler, reflection::ClassInfoPtr, reflection::MethodInfoPtr, reflection::MemberInfoPtr>;

    ReflectedObject(DataType = DataType());
    auto& GetValue() {return m_value;}
    auto& GetValue() const {return m_value;}

private:
    DataType m_value;
};

} // interpreter
} // codegen

#endif // REFLECTED_OBJECT_H

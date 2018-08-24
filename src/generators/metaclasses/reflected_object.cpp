#include "reflected_object.h"

namespace codegen
{
namespace interpreter
{

ReflectedObject::ReflectedObject(DataType val)
    : m_value(std::move(val))
{

}

} // interpreter
} // codegen

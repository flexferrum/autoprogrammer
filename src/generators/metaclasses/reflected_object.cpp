#include "reflected_object.h"

#include <iostream>

namespace codegen
{
namespace interpreter
{

ReflectedObject::ReflectedObject(DataType val)
    : m_value(std::move(val))
{

}

bool ReflectedMethods::Compiler_message(InterpreterImpl* interpreter, const Compiler& obj, const std::string& msg)
{
    std::cout << "###### " << msg << std::endl;
    return true;
}

} // interpreter
} // codegen

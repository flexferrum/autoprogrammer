#ifndef VALUE_OPS_H
#define VALUE_OPS_H

#include <clang/AST/APValue.h>
#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/APSInt.h>

#include "reflected_object.h"
#include "value.h"

namespace codegen
{
namespace interpreter
{

class InterpreterImpl;

namespace value_ops
{
bool ConvertToBool(InterpreterImpl* interpreter, const Value& obj);
bool CallMember(InterpreterImpl* interpreter, Value& obj, const clang::CXXMethodDecl* method, const std::vector<Value>& args, Value& result);
} // namespace value_ops

} // interpreter
} // codegen


#endif // VALUE_OPS_H

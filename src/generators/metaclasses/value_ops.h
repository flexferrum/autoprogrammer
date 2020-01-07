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
enum BinaryOp 
{
    Plus,
};

bool ConvertToBool(InterpreterImpl* interpreter, const Value& obj);
Value DoBinaryOperation(const Value& left, const Value& right, BinaryOp op);
bool CallMember(InterpreterImpl* interpreter, Value& obj, const clang::CXXMethodDecl* method, const std::vector<Value>& args, Value& result);
bool CallFunction(InterpreterImpl* interpreter, const clang::FunctionDecl* method, const std::vector<Value>& args, Value& result);

} // namespace value_ops

} // interpreter
} // codegen


#endif // VALUE_OPS_H

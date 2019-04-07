#include "reflected_object.h"
#include "value.h"
#include "cpp_interpreter_impl.h"
#include "reflected_range.h"


#include <iostream>

namespace codegen
{
namespace interpreter
{



ReflectedObject::ReflectedObject(DataType val)
    : m_value(std::move(val))
{

}

bool ReflectedMethods::Compiler_message(InterpreterImpl* interpreter, const Compiler& obj, Value& result, const std::string& msg)
{
    interpreter->Report(MessageType::Notice, clang::SourceLocation(), msg);
    result = VoidValue();
    return true;
}

bool ReflectedMethods::Compiler_require(InterpreterImpl* interpreter, const Compiler& obj, Value& result, bool testResult, const std::string& msg)
{
    result = VoidValue();
    if (!testResult)
        std::cout << "###### " << msg << std::endl;

    return true;
}

bool ReflectedMethods::ClassInfo_variables(InterpreterImpl* interpreter, reflection::ClassInfoPtr obj, Value& result)
{
    auto rangePtr = MakeStdCollectionRefRange(obj->members);
    result = Value(ReflectedObject(rangePtr));

    std::cout << "#### Range to ClassInfo::variables created" << std::endl;
    return true;
}

bool ReflectedMethods::ClassInfo_functions(InterpreterImpl* interpreter, reflection::ClassInfoPtr obj, Value& result)
{
    auto methods = obj->methods;
    auto rangePtr = MakeStdCollectionRefRange(std::move(methods));
    result = Value(ReflectedObject(rangePtr));

    std::cout << "#### Range to ClassInfo::functions created" << std::endl;
    return true;
}

bool ReflectedMethods::ClassInfo_addMethod(InterpreterImpl* interpreter, reflection::ClassInfoPtr obj, reflection::MethodInfoPtr method, Value& result)
{
    result = Value(VoidValue());
    obj->methods.push_back(method);

    return true;
}

bool ReflectedMethods::MethodInfo_is_implicit(InterpreterImpl* interpreter, reflection::MethodInfoPtr obj, Value& result)
{
    result = Value(obj->isImplicit);

    std::cout << "#### ReflectedMethods::MethodInfo_is_implicit called. IsImplicit: " << obj->isImplicit << std::endl;
    return true;
}

bool ReflectedMethods::MethodInfo_is_copy_ctor(InterpreterImpl* interpreter, reflection::MethodInfoPtr obj, Value& result)
{
    result = Value(obj->constructorType == reflection::ConstructorType::Copy);

    std::cout << "#### ReflectedMethods::MethodInfo_is_copy_ctor called. Object: " << obj << std::endl;
    return true;
}

bool ReflectedMethods::MethodInfo_is_move_ctor(InterpreterImpl* interpreter, reflection::MethodInfoPtr obj, Value& result)
{
    result = Value(obj->constructorType == reflection::ConstructorType::Move);

    std::cout << "#### ReflectedMethods::MethodInfo_is_move_ctor called. Object: " << obj << std::endl;
    return true;
}

bool ReflectedMethods::MethodInfo_is_public(InterpreterImpl* interpreter, reflection::MethodInfoPtr obj, Value& result)
{
    result = Value(obj->accessType == reflection::AccessType::Public);

    std::cout << "#### ReflectedMethods::MethodInfo_is_public called. Object: " << obj << std::endl;
    return true;
}

bool ReflectedMethods::ClassMemberBase_has_access(InterpreterImpl* interpreter, reflection::MethodInfoPtr obj, Value& result)
{
    result = Value(obj->accessType != reflection::AccessType::Undefined);

    std::cout << "#### ReflectedMethods::ClassMemberBase_has_access called. Object: " << obj << std::endl;
    return true;
}

bool ReflectedMethods::ClassMemberBase_name(InterpreterImpl* interpreter, reflection::NamedDeclInfoPtr obj, Value& result)
{
    result = Value(std::string(obj->name));

    std::cout << "#### ReflectedMethods::ClassMemberBase_name called. Object: " << obj << std::endl;
    return true;
}

bool ReflectedMethods::ClassMemberBase_make_public(InterpreterImpl* interpreter, reflection::MethodInfoPtr obj, Value& result)
{
    obj->accessType = reflection::AccessType::Public;
    result = Value(VoidValue());
    return true;
}

bool ReflectedMethods::MethodInfo_make_pure_virtual(InterpreterImpl* interpreter, reflection::MethodInfoPtr obj, Value& result)
{
    result = Value(VoidValue());
    obj->isPure = true;
    obj->isVirtual = true;

    std::cout << "#### ReflectedMethods::MethodInfo_make_pure_virtual. Object: " << obj << std::endl;
    return true;
}

bool ReflectedMethods::RangeT_empty(InterpreterImpl* interpreter, RangeTPtr obj, Value& result)
{
    std::cout << "#### ReflectedMethods::RangeT_empty called. Object: " << obj << std::endl;
    result = Value(obj->Empty());

    return true;
}

bool ReflectedMethods::RangeT_begin(InterpreterImpl* interpreter, RangeTPtr obj, Value& result)
{
    result = Value(obj->Begin());

    std::cout << "#### ReflectedMethods::RangeT_begin called. Object type: " << result.GetValue().which() << std::endl;
    return true;
}

bool ReflectedMethods::RangeT_end(InterpreterImpl* interpreter, RangeTPtr obj, Value& result)
{
    result = Value(obj->End());

    std::cout << "#### ReflectedMethods::RangeT_end called. Object type: " << result.GetValue().which() << std::endl;
    return true;
}

bool ReflectedMethods::IteratorT_OperNotEqual_Same(InterpreterImpl* interpreter, IteratorTPtr left, Value& result, IteratorTPtr right)
{
    result = Value(!left->IsEqual(right.get()));

    std::cout << "#### IteratorT_OperNotEqual called. Object: " << left << std::endl;
    return true;
}

bool ReflectedMethods::IteratorT_OperStar(InterpreterImpl* interpreter, IteratorTPtr left, Value& result)
{
    result = Value(left->GetValue());

    std::cout << "#### IteratorT_IteratorT_OperStar called." << std::endl;
    return true;
}

bool ReflectedMethods::IteratorT_OperPrefixInc(InterpreterImpl* interpreter, IteratorTPtr left, Value& result)
{
    left->PrefixInc();
    result = Value(left);

    std::cout << "#### IteratorT_IteratorT_OperPrefixInc called." << std::endl;
    return true;
}
} // interpreter
} // codegen

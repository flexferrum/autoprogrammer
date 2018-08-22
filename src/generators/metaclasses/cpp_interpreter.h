#ifndef CPP_INTERPRETER_H
#define CPP_INTERPRETER_H

#include "generators/basic_generator.h"
#include "decls_reflection.h"

namespace codegen
{
class CppInterpreter
{
public:
    explicit CppInterpreter(const clang::ASTContext* astContext);
    ~CppInterpreter();

    void Execute(reflection::ClassInfoPtr metaclass, reflection::ClassInfoPtr inst, reflection::MethodInfoPtr generator);

private:
    const clang::ASTContext* m_astContext = nullptr;
};
} // codegen

#endif // CPP_INTERPRETER_H

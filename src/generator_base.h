#ifndef GENERATOR_BASE_H
#define GENERATOR_BASE_H

#include "options.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace codegen
{
class GeneratorBase
{
public:
    virtual ~GeneratorBase() {}
    
    virtual void SetupMatcher(clang::ast_matchers::MatchFinder& finder, clang::ast_matchers::MatchFinder::MatchCallback* defaultCallback) = 0;
    virtual void OnCompilationStarted() = 0;
    virtual void HandleMatch(const clang::ast_matchers::MatchFinder::MatchResult& matchResult) = 0;
    virtual bool Validate() = 0;
    virtual bool GenerateOutput() = 0;
};

using GeneratorPtr = std::unique_ptr<GeneratorBase>;

using GeneratorFactory = GeneratorPtr (*)(const Options& opts);
} // codegen

#endif // GENERATOR_BASE_H

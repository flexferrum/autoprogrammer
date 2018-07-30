#include "metaclasses_generator.h"

#include <clang/ASTMatchers/ASTMatchers.h>

using namespace clang::ast_matchers;

namespace codegen
{

MetaclassesGenerator::MetaclassesGenerator(const Options& opts)
    : BasicGenerator(opts)
{

}

void MetaclassesGenerator::SetupMatcher(clang::ast_matchers::MatchFinder& finder, clang::ast_matchers::MatchFinder::MatchCallback* defaultCallback)
{
}

void MetaclassesGenerator::HandleMatch(const clang::ast_matchers::MatchFinder::MatchResult& matchResult)
{
}

bool MetaclassesGenerator::Validate()
{
    return true;
}

void MetaclassesGenerator::WriteHeaderContent(codegen::CppSourceStream& hdrOs)
{
}
} // codegen

codegen::GeneratorPtr CreateMetaclassesGen(const codegen::Options& opts)
{
    return std::make_unique<codegen::MetaclassesGenerator>(opts);
}


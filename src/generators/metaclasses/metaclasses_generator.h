#ifndef METACLASSES_GENERATOR_H
#define METACLASSES_GENERATOR_H

#include "generators/basic_generator.h"
#include "decls_reflection.h"

namespace codegen
{

class MetaclassesGenerator : public BasicGenerator
{
public:
    MetaclassesGenerator(const Options& opts);

    // GeneratorBase interface
    void SetupMatcher(clang::ast_matchers::MatchFinder& finder, clang::ast_matchers::MatchFinder::MatchCallback* defaultCallback) override;
    void HandleMatch(const clang::ast_matchers::MatchFinder::MatchResult& matchResult) override;
    bool Validate() override;

protected:
    // BasicGenerator interface
    void WriteHeaderContent(CppSourceStream& hdrOs) override;

    void ProcessMetaclassDecl(reflection::ClassInfoPtr classInfo, const clang::ASTContext* astContext);

private:
    reflection::NamespacesTree m_namespaces;

};

} // codegen

#endif // METACLASSES_GENERATOR_H

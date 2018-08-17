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
    struct MetaclassImplInfo
    {
        reflection::ClassInfoPtr metaclass;
        reflection::ClassInfoPtr implDecl;
        reflection::ClassInfoPtr impl;
    };

    // BasicGenerator interface
    void WriteHeaderContent(CppSourceStream& hdrOs) override;

    void ProcessMetaclassDecl(reflection::ClassInfoPtr classInfo, const clang::ASTContext* astContext);
    void ProcessMetaclassImplDecl(reflection::ClassInfoPtr classInfo, const clang::ASTContext* astContext);    

private:

    std::vector<MetaclassImplInfo> m_implsInfo;
    reflection::NamespacesTree m_namespaces;
    reflection::NamespacesTree m_implNamespaces;

};

} // codegen

#endif // METACLASSES_GENERATOR_H

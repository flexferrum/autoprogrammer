#include "pimpl_generator.h"
#include "options.h"
#include "ast_reflector.h"
#include "ast_utils.h"
#include "utils.h"

#include <clang/ASTMatchers/ASTMatchers.h>

#include <iostream>

using namespace clang::ast_matchers;

namespace codegen 
{
namespace  
{
DeclarationMatcher pimplMatcher = cxxRecordDecl
        (isExpansionInMainFile(), isDerivedFrom("flex_lib::pimpl")).bind("pimpl");
}


PimplGenerator::PimplGenerator(const codegen::Options &opts)
    : BasicGenerator(opts)
{
    
}

void PimplGenerator::SetupMatcher(clang::ast_matchers::MatchFinder &finder, clang::ast_matchers::MatchFinder::MatchCallback *defaultCallback)
{
    finder.addMatcher(pimplMatcher, defaultCallback);    
}

void PimplGenerator::HandleMatch(const clang::ast_matchers::MatchFinder::MatchResult &matchResult)
{
    if (const clang::CXXRecordDecl* decl = matchResult.Nodes.getNodeAs<clang::CXXRecordDecl>("pimpl"))
    {
        reflection::AstReflector reflector(matchResult.Context);
        
        auto ci = reflector.ReflectClass(decl, &m_namespaces);
        
        std::cout << "### Pimpl declaration found: " << ci->GetFullQualifiedName(false) << std::endl;
    }        
}

bool PimplGenerator::Validate()
{
    return true;    
}

void PimplGenerator::WriteHeaderPreamble(CppSourceStream &hdrOs)
{
    hdrOs << out::header_guard(m_options.outputHeaderName) << "\n";
    
    // Include input files (directly, by path)
    for (auto fileName : m_options.inputFiles)
    {
        std::replace(fileName.begin(), fileName.end(), '\\', '/');
        hdrOs << "#include \"" << fileName << "\"\n";
    }
    
    WriteExtraHeaders(hdrOs);

    // Necessary library files
    hdrOs << "#include <utility>\n\n";
}

void PimplGenerator::WriteHeaderPostamble(CppSourceStream &hdrOs)
{
    hdrOs << out::scope_exit;    
}

void PimplGenerator::WriteHeaderContent(CppSourceStream &hdrOs)
{
    WriteNamespaceContents(hdrOs, m_namespaces.GetRootNamespace(), [this](CppSourceStream &os, reflection::NamespaceInfoPtr ns) {
        for (auto& classInfo : ns->classes)
        {
            WritePimplImplementation(os, classInfo);
        }      
    });    
}

void PimplGenerator::WritePimplImplementation(CppSourceStream& os, reflection::ClassInfoPtr classInfo)
{
    for (reflection::MethodInfoPtr methodInfo : classInfo->methods)
    {
        if (methodInfo->isCtor)
            WriteCtorImplementation(os, classInfo, methodInfo);
        else if (methodInfo->isDtor)
        {
            os << out::new_line;
            if (methodInfo->isNoExcept)
                os << methodInfo->GetScopedName() << "() noexcept = default;\n";
            else
                os << methodInfo->GetScopedName() << "() = default;\n";
        }
        else
            WriteMethodImplementation(os, classInfo, methodInfo);
    }
}

void PimplGenerator::WriteCtorImplementation(CppSourceStream& os, reflection::ClassInfoPtr classInfo, reflection::MethodInfoPtr methodInfo)
{
    if (methodInfo->isImplicit)
        return;
    
    os << out::new_line << methodInfo->GetScopedName() << "(";
    WriteSeq(os, methodInfo->params, ", ", [](auto&& os, const reflection::MethodParamInfo& param) {os << param.fullDecl;});
    os << ")";
    if (methodInfo->isNoExcept)
        os << " noexcept";
    os << out::new_line(+1) << ": pimpl(this";
    bool isFirst = true;
    WriteSeq(os, methodInfo->params, ", ", [&isFirst](auto&& os, const reflection::MethodParamInfo& param) 
    {
        if (isFirst)
        {
            isFirst = false;
            os << ", ";
        }
        reflection::TypeInfoPtr pType = param.type;
        
        if (pType->canBeMoved())
            os << "std::move(" << param.name << ")";
        else
            os << param.name;
    });
    os << ")";
    os << out::new_line << "{";
    os << out::new_line << "}\n";
}

void PimplGenerator::WriteMethodImplementation(CppSourceStream& os, reflection::ClassInfoPtr classInfo, reflection::MethodInfoPtr methodInfo)
{
    if (methodInfo->isImplicit || methodInfo->isStatic)
        return;
    
    os << out::new_line << methodInfo->returnType->getPrintedName() << " " << methodInfo->GetScopedName() << "(";
    WriteSeq(os, methodInfo->params, ", ", [](auto&& os, const reflection::MethodParamInfo& param) {os << param.fullDecl;});
    os << ")";
    if (methodInfo->isConst)
        os << " const";
    if (methodInfo->isNoExcept)
        os << " noexcept";
    out::BracedStreamScope body("", "\n");
    os << body;
    os << out::new_line;
    const reflection::BuiltinType* retType = methodInfo->returnType->getAsBuiltin();
    if (retType == nullptr || retType->type != reflection::BuiltinType::Void)
        os << "return ";
    os << "m_impl->" << methodInfo->name << "(";
    WriteSeq(os, methodInfo->params, ", ", [classInfo](auto&& os, const reflection::MethodParamInfo& param) 
    {
        std::string paramName = param.name;
        
        reflection::TypeInfoPtr pType = param.type;
        const reflection::RecordType* rType = pType->getAsRecord();
        if (rType && rType->decl == classInfo->decl)
            paramName = "*" + param.name + (pType->getPointingLevels() != 0 ? "->" : ".") + "m_impl.get()";
        
        if (pType->canBeMoved())
            os << "std::move(" << paramName << ")";
        else
            os << paramName;
    });
    os << ");";
}

void PimplGenerator::WriteSourcePreamble(CppSourceStream &srcOs)
{
    
}

void PimplGenerator::WriteSourceContent(CppSourceStream &srcOs)
{
    
}

void PimplGenerator::WriteSourcePostamble(CppSourceStream &srcOs)
{
    
}

} // codegen

codegen::GeneratorPtr CreatePimplGen(const codegen::Options& opts)
{
    return std::make_unique<codegen::PimplGenerator>(opts);
}

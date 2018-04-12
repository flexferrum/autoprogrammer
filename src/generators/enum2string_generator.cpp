#include "enum2string_generator.h"
#include "options.h"
#include "ast_reflector.h"
#include "ast_utils.h"

#include <clang/ASTMatchers/ASTMatchers.h>

#include <iostream>

using namespace clang::ast_matchers;

namespace codegen 
{
namespace  
{
DeclarationMatcher enumMatcher = 
        enumDecl(isExpansionInMainFile()).bind("enum");
}

Enum2StringGenerator::Enum2StringGenerator(const Options &opts)
    : BasicGenerator(opts)
{
    
}

void Enum2StringGenerator::SetupMatcher(clang::ast_matchers::MatchFinder &finder, clang::ast_matchers::MatchFinder::MatchCallback *defaultCallback)
{
    finder.addMatcher(enumMatcher, defaultCallback);    
}

void Enum2StringGenerator::HandleMatch(const clang::ast_matchers::MatchFinder::MatchResult &matchResult)
{
    if (const clang::EnumDecl* decl = matchResult.Nodes.getNodeAs<clang::EnumDecl>("enum"))
    {
        reflection::AstReflector reflector(matchResult.Context);
        
        reflector.ReflectEnum(decl, &m_namespaces);
    }    
}

void Enum2StringGenerator::WriteHeaderPreamble(CppSourceStream &hdrOs)
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
    hdrOs << "#include <flex_lib/stringized_enum.h>\n";
    hdrOs << "#include <algorithm>\n";
    hdrOs << "#include <utility>\n\n";
}

void Enum2StringGenerator::WriteHeaderPostamble(CppSourceStream &hdrOs)
{
    hdrOs << out::scope_exit;
}

namespace
{
out::ScopedParams MakeScopedParams(CppSourceStream &os, reflection::EnumInfoPtr enumDescr)
{
    std::string scopeSpec = enumDescr->scopeSpecifier;
    std::string scopedName =  scopeSpec + (scopeSpec.empty() ? "" : "::") + enumDescr->name;
    std::string fullQualifiedName = enumDescr->GetFullQualifiedName(false);

    return out::ScopedParams(os, {
        {"enumName", enumDescr->name},
        {"enumScopedName", scopedName},
        {"scopeQual", scopeSpec},
        {"namespaceQual", enumDescr->namespaceQualifier},
        {"enumFullQualifiedName", fullQualifiedName},                                 
        {"prefix", enumDescr->isScoped ? scopedName + "::" : scopeSpec + (scopeSpec.empty() ? "" : "::")}
    });    
}

}

void Enum2StringGenerator::WriteHeaderContent(CppSourceStream &hdrOs)
{
    std::vector<reflection::EnumInfoPtr> enums;
    WriteNamespaceContents(hdrOs, m_namespaces.GetRootNamespace(), [this, &enums](CppSourceStream &os, reflection::NamespaceInfoPtr ns) {
        for (auto& enumInfo : ns->enums)
        {
            WriteEnumToStringConversion(os, enumInfo);
            WriteEnumFromStringConversion(os, enumInfo);
            enums.push_back(enumInfo);
        }      
    });

    hdrOs << "\n\n";
    
    for (reflection::EnumInfoPtr enumInfo : enums)
    {
        auto scopedParams = MakeScopedParams(hdrOs, enumInfo);

        {
            hdrOs << out::new_line << "template<>";
            out::BracedStreamScope body("inline const char* flex_lib::Enum2String($enumFullQualifiedName$ e)", "\n");
            hdrOs << out::new_line << body;
            hdrOs << out::new_line << "return $namespaceQual$::$enumName$ToString(e);";
        }
        {
            hdrOs << out::new_line << "template<>";
            out::BracedStreamScope body("inline $enumFullQualifiedName$ flex_lib::String2Enum<$enumFullQualifiedName$>(const char* itemName)", "\n");
            hdrOs << out::new_line << body;
            hdrOs << out::new_line << "return $namespaceQual$::StringTo$enumName$(itemName);";
        }
    }
    {
        out::BracedStreamScope flNs("\nnamespace std", "\n\n", 0);
        hdrOs << out::new_line << flNs;
        
        for (reflection::EnumInfoPtr enumInfo : enums)
        {
            auto scopedParams = MakeScopedParams(hdrOs, enumInfo);

            out::BracedStreamScope body("inline std::string to_string($enumFullQualifiedName$ e)", "\n");
            hdrOs << out::new_line << body;
            hdrOs << out::new_line << "return $namespaceQual$::$enumName$ToString(e);";
        }
    }
}

// Enum item to string conversion writer
void Enum2StringGenerator::WriteEnumToStringConversion(CppSourceStream &hdrOs, const reflection::EnumInfoPtr &enumDescr)
{
    auto scopedParams = MakeScopedParams(hdrOs, enumDescr);
    
    out::BracedStreamScope fnScope("inline const char* $enumName$ToString($enumScopedName$ e)", "\n");
    hdrOs << out::new_line << fnScope;
    {
        out::BracedStreamScope switchScope("switch (e)", "\n");
        hdrOs << out::new_line << switchScope;
        out::OutParams innerParams;
        for (auto& i : enumDescr->items)
        {
            innerParams["itemName"] = i.itemName;
            hdrOs << out::with_params(innerParams)
                  << out::new_line(-1) << "case $prefix$$itemName$:"
                  << out::new_line << "return \"$itemName$\";";
        }
    }
    hdrOs << out::new_line << "return \"Unknown Item\";";
}

// String to enum conversion writer
void Enum2StringGenerator::WriteEnumFromStringConversion(CppSourceStream &hdrOs, const reflection::EnumInfoPtr &enumDescr)
{
    auto params = MakeScopedParams(hdrOs, enumDescr);
    
    out::BracedStreamScope fnScope("inline $enumScopedName$ StringTo$enumName$(const char* itemName)", "\n");
    hdrOs << out::new_line << fnScope;
    {
        out::BracedStreamScope itemsScope("static std::pair<const char*, $enumScopedName$> items[] = ", ";\n");
        hdrOs << out::new_line << itemsScope;

        out::OutParams& innerParams = params.GetParams();
        auto items = enumDescr->items;
        std::sort(begin(items), end(items), [](auto& i1, auto& i2) {return i1.itemName < i2.itemName;});
        for (auto& i : items)
        {
            innerParams["itemName"] = i.itemName;
            hdrOs << out::with_params(innerParams) << out::new_line << "{\"$itemName$\", $prefix$$itemName$},";
        }    
    }

    hdrOs << out::with_params(params.GetParams()) << R"(
     $enumScopedName$ result;
     if (!flex_lib::detail::String2Enum(itemName, items, result))
         flex_lib::bad_enum_name::Throw(itemName, "$enumName$");
 
     return result;)";
}
} // codegen

codegen::GeneratorPtr CreateEnum2StringGen(const codegen::Options& opts)
{
    return std::make_unique<codegen::Enum2StringGenerator>(opts);
}

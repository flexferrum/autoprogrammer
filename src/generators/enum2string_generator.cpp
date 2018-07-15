#include "enum2string_generator.h"
#include "options.h"
#include "ast_reflector.h"
#include "ast_utils.h"
#include "jinja2_reflection.h"

#include <clang/ASTMatchers/ASTMatchers.h>

#include <iostream>

using namespace clang::ast_matchers;

namespace codegen
{
namespace
{
DeclarationMatcher enumMatcher =
        enumDecl().bind("enum");

auto g_enum2stringTemplate =
R"(
{% extends "header_skeleton.j2tpl" %}
{% block generator_headers %}
 #include <flex_lib/stringized_enum.h>
 #include <algorithm>
 #include <utility>
{% endblock %}

{% block namespaced_decls %}{{super()}}{% endblock %}

{% block namespace_content %}
{% for enum in ns.enums | sort(attribute="name") %}
{% set enumName = enum.name %}
{% set scopeSpec = enum.scopeSpecifier %}
{% set scopedName = scopeSpec ~ ('::' if scopeSpec) ~ enumName %}
{% set prefix = (scopedName + '::') if not enumInfo.isScoped else (scopedName ~ '::' ~ scopeSpec ~ ('::' if scopeSpec)) %}

inline const char* {{enumName}}ToString({{scopedName}} e)
{
    switch (e)
    {
{% for itemName in enum.items | map(attribute="itemName") | sort%}
    case {{prefix}}{{itemName}}:
        return "{{itemName}}";
{% endfor %}
    }
    return "Unknown Item";
}

inline {{scopedName}} StringTo{{enumName}}(const char* itemName)
{
    static std::pair<const char*, {{scopedName}}> items[] = {
{% for itemName in enum.items | map(attribute="itemName") | sort %}
        {"{{itemName}}", {{prefix}}{{itemName}} } {{',' if not loop.last }}
{% endfor %}
    };

    {{scopedName}} result;
    if (!flex_lib::detail::String2Enum(itemName, items, result))
         flex_lib::bad_enum_name::Throw(itemName, "{{enumName}}");

    return result;
}
{% endfor %}{% endblock %}

{% block global_decls %}
{% for ns in [rootNamespace] recursive %}
{% for enum in ns.enums %}

template<>
inline const char* flex_lib::Enum2String({{enum.fullQualifiedName}} e)
{
    return {{enum.namespaceQualifier}}::{{enum.name}}ToString(e);
}

template<>
inline {{enum.fullQualifiedName}} flex_lib::String2Enum<{{enum.fullQualifiedName}}>(const char* itemName)
{
    return {{enum.namespaceQualifier}}::StringTo{{enum.name}}(itemName);
}

inline std::string to_string({{enum.fullQualifiedName}} e)
{
    return {{enum.namespaceQualifier}}::{{enum.name}}ToString(e);
}
{% endfor %}
{{loop(ns.namespaces)}}
{% endfor %}
{% endblock %}
)";
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
        if (!IsFromInputFiles(decl->getLocStart(), matchResult.Context))
            return;

        reflection::AstReflector reflector(matchResult.Context);

        reflector.ReflectEnum(decl, &m_namespaces);
    }
}

void Enum2StringGenerator::WriteHeaderPreamble(CppSourceStream &hdrOs)
{
    jinja2::ValuesMap params = {
        {"inputFiles", jinja2::Reflect(m_options.inputFiles)},
        {"extraHeaders", jinja2::Reflect(m_options.extraHeaders)},
    };
    m_headerPreambleTpl.Render(hdrOs, params);
}

void Enum2StringGenerator::WriteHeaderPostamble(CppSourceStream &hdrOs)
{
//    hdrOs << out::scope_exit;
}

void Enum2StringGenerator::WriteHeaderContent(CppSourceStream &hdrOs)
{
    jinja2::Template tpl(&m_templateEnv);
    tpl.Load(g_enum2stringTemplate);

    jinja2::ValuesMap params = {
        {"inputFiles", jinja2::Reflect(m_options.inputFiles)},
        {"headerGuard", GetHeaderGuard(m_options.outputHeaderName)},
        {"rootNamespace", jinja2::Reflect(m_namespaces.GetRootNamespace())}
    };

    SetupCommonTemplateParams(params);

    tpl.Render(hdrOs, params);
}
} // codegen

codegen::GeneratorPtr CreateEnum2StringGen(const codegen::Options& opts)
{
    return std::make_unique<codegen::Enum2StringGenerator>(opts);
}

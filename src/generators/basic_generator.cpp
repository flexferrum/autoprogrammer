#include "basic_generator.h"
#include "decls_reflection.h"

#include <clang/AST/ASTContext.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/FileManager.h>
#include <clang/Rewrite/Frontend/Rewriters.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Format/Format.h>

#include <llvm/Support/Path.h>
#include <llvm/Support/JamCRC.h>
#include <llvm/ADT/ArrayRef.h>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include <jinja2cpp/reflected_value.h>

#include <fstream>
#include <sstream>

namespace codegen
{
auto g_commonMacros = 
R"(
{% macro ProcessTypedMember(type, macroPrefix, extraVal) %}
	{% if type.type.isNoType %}
	{% elif type.type.isBuiltinType %}
	{{ extraVal | applymacro(macro=macroPrefix + 'BuiltinType', type ) }}
	{% elif type.type.isRecordType %}
	{{ extraVal | applymacro(macro=macroPrefix + 'RecordType', type ) }}
	{% elif type.type.isTemplateType %}
	{{ extraVal | applymacro(macro=macroPrefix + 'TemplateType', type ) }}
	{% elif type.type.isWellKnownType %}
	{{ extraVal | applymacro(macro=macroPrefix + 'WellKnownType', type ) }}
	{% elif type.type.isArrayType %}
	{{ extraVal | applymacro(macro=macroPrefix + 'ArrayType', type, type.type.dims, type.type.itemType ) }}
	{% elif type.type.isEnumType %}
	{{ extraVal | applymacro(macro=macroPrefix + 'EnumType', type ) }}
	{% else %}
	{{ extraVal | applymacro(macro=macroPrefix + 'GenericType', type) }}
	{% endif %}
{% endmacro %}

)";

auto g_headerSkeleton =
  R"(
{% if headerGuard is defined %}
 #ifndef {{headerGuard}}
 #define {{headerGuard}}
{% else %}
 #pragma once
{% endif %}

{% for fileName in options.input_files | sort %}
 #include "{{fileName}}"
{% endfor %}

{% for fileName in options.extra_headers | sort %}
{% if fileName is startsWith('<') %}
 #include {{fileName}}
{% else %}
 #include "{{fileName}}"
{% endif %}
{% endfor %}

{% block generator_headers scoped %}{% endblock %}
{% block namespaced_decls scoped %}
{% set ns = rootNamespace %}
{# {ns | pprint}}
{{rootNamespace | pprint} #}
{% block namespace_content scoped %}{%endblock%}
{% for ns in rootNamespace.innerNamespaces recursive %}namespace {{ns.name}}
{
{{self.namespace_content()}}
{{ loop(ns.innerNamespaces) }}
}
{% endfor %}
{% endblock %}

{% block global_decls scoped %}{% endblock %}

{% if headerGuard is defined %}
 #endif // {{headerGuard}}
{% endif %}
)";


BasicGenerator::BasicGenerator(const Options &opts)
    : m_options(opts)
{

}

void BasicGenerator::OnCompilationStarted()
{

}

void BasicGenerator::SetupTemplate(jinja2::TemplateEnv* env, std::string templateName) 
{
    m_templateEnv = env;
    m_templateName = templateName;
    m_templateEnv->AddFilesystemHandler(std::string(), m_inMemoryTemplates);
    m_inMemoryTemplates.AddFile("header_skeleton.j2tpl", g_headerSkeleton);
    m_inMemoryTemplates.AddFile("common_macros.j2tpl", g_commonMacros);
}

bool BasicGenerator::Validate()
{
    return true;
}

enum class FileState
{
    Good,
    Bad,
    NoFile
};

FileState OpenGeneratedFile(const std::string& fileName, std::ofstream& fileStream, std::ostream*& targetStream)
{
    if (fileName == "stdout")
    {
        targetStream = &std::cout;
        return FileState::Good;
    }

    if (fileName.empty())
        return FileState::NoFile;

    fileStream.open(fileName, std::ios_base::out | std::ios_base::trunc);
    if (!fileStream.good())
        return FileState::Bad;

    targetStream = &fileStream;
    return FileState::Good;
}

bool BasicGenerator::GenerateOutput(const clang::ASTContext* astContext, clang::SourceManager* sourceManager)
{
    if (!clang::format::getPredefinedStyle(m_options.formatStyleName, clang::format::FormatStyle::LK_Cpp, &m_formatStyle))
    {
        std::cout << "Can't load style with name: " << m_options.formatStyleName << std::endl;
    }
    bool result = true;
    if (!m_options.outputHeaderName.empty())
    {
        result = GenerateOutputFile(m_options.outputHeaderName, "fl-codegen-output-header.h", astContext, sourceManager, [this](auto& stream) {
            this->WriteHeaderPreamble(stream);
            this->WriteHeaderContent(stream);
            this->WriteHeaderPostamble(stream);

            return true;
        });
    }
    if (!result)
        return false;

    if (!m_options.outputSourceName.empty())
    {
        result = GenerateOutputFile(m_options.outputSourceName, "fl-codegen-output-source.cpp", astContext, sourceManager, [this](auto& stream) {
            this->WriteSourcePreamble(stream);
            this->WriteSourceContent(stream);
            this->WriteSourcePostamble(stream);

            return true;
        });
    }

    return result;
}

bool BasicGenerator::GenerateOutputFile(const std::string& fileName, std::string tmpFileId, const clang::ASTContext* astContext, clang::SourceManager* sourceManager, std::function<bool (CppSourceStream&)> generator)
{
    using namespace clang;

    std::ostringstream targetHeaderOs;
    {
        CppSourceStream stream(targetHeaderOs);

        if (!generator(stream))
            return false;
    }

    auto fileContent = targetHeaderOs.str();
    tooling::Range fileRange(0, static_cast<unsigned>(fileContent.size()));
    auto replaces = format::reformat(m_formatStyle, fileContent, {fileRange});
    auto formattingResult = tooling::applyAllReplacements(fileContent, replaces);

    if (!formattingResult)
        return false;
    std::string formattedContent = formattingResult.get();

    std::ofstream outHdrFile;
    std::ostream* targetOs;

    auto hdrFileState = OpenGeneratedFile(fileName, outHdrFile, targetOs);
    if (hdrFileState == FileState::Bad)
    {
        std::cerr << "Can't open output file for writing: " << fileName << std::endl;
        return false;
    }
    *targetOs << formattedContent;

    return true;
}

bool BasicGenerator::IsFromInputFiles(const clang::SourceLocation& loc, const clang::ASTContext* context) const
{
    auto& sm = context->getSourceManager();
    auto ploc = sm.getPresumedLoc(loc, false);
    std::string locFilename = ploc.getFilename();

    return m_options.inputFiles.count(locFilename) != 0;
}

bool BasicGenerator::IsFromUpdatingFile(const clang::SourceLocation& loc, const clang::ASTContext* context) const
{
    auto& sm = context->getSourceManager();
    auto ploc = sm.getPresumedLoc(loc, false);
    std::string locFilename = ploc.getFilename();

    return m_options.fileToUpdate == locFilename;
}

void BasicGenerator::WriteExtraHeaders(CppSourceStream& os)
{
    for (auto& h : m_options.extraHeaders)
    {
        os << "#include ";
        bool quoted = h[0] != '<';
        os << (quoted ? "\"" : "") << h << (quoted ? "\"" : "") << "\n";
    }
}

std::string BasicGenerator::GetHeaderGuard(const std::string& filePath)
{
    std::string fileName = llvm::sys::path::filename(filePath).str();
    boost::algorithm::replace_all(fileName, ".", "_");
    boost::algorithm::replace_all(fileName, "-", "_");
    boost::algorithm::replace_all(fileName, " ", "_");
    boost::algorithm::to_upper(fileName);
    llvm::JamCRC crcCalc;
    crcCalc.update(llvm::ArrayRef<char>(filePath.data(), filePath.size()));
    uint32_t fileHash = crcCalc.getCRC();

    std::ostringstream str;
    str << "_" << fileName << "_" << fileHash;
    return str.str();
}

void BasicGenerator::RenderTemplate(std::ostream& os, jinja2::ValuesMap& params)
{
    auto loadRes = m_templateEnv->LoadTemplate(m_templateName);
    if (!loadRes)
    {
        Report(MessageType::Fatal, "", 0, 0, "Template load error: " + loadRes.error().ToString());
        return;
    }

    DoTemplateRender(loadRes.value(), os, params);
}

void BasicGenerator::RenderStringTemplate(const char* tplString, std::ostream& os, jinja2::ValuesMap& params)
{
    jinja2::Template tpl(m_templateEnv);

    auto loadRes = tpl.Load(tplString);
    if (!loadRes)
    {
        Report(MessageType::Fatal, "", 0, 0, "Template load error: " + loadRes.error().ToString());
        return;
    }

    DoTemplateRender(tpl, os, params);
}

void BasicGenerator::DoTemplateRender(jinja2::Template& tpl, std::ostream& os, jinja2::ValuesMap& params)
{
    auto renderRes = tpl.Render(os, params);
    if (!renderRes)
    {
        Report(MessageType::Fatal, "", 0, 0, "Template render error: " + renderRes.error().ToString());
        return;
    }
}

void BasicGenerator::Report(MessageType type, const std::string fileName, unsigned line, unsigned col, std::string message)
{
    m_options.consoleWriter->WriteMessage(type, fileName, line, col, message);
}

void BasicGenerator::Report(MessageType type, const reflection::SourceLocation& loc, std::string message)
{
    Report(type, loc.fileName, loc.line, loc.column, std::move(message));
}

void BasicGenerator::Report(MessageType type, const clang::SourceLocation& loc, const clang::ASTContext* astContext, std::string message)
{
    // TODO:
}



} // codegen

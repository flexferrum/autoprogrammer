#include "jinja2_reflector_generator.h"
#include "options.h"
#include "ast_reflector.h"
#include "ast_utils.h"
#include "jinja2_reflection.h"

#include <clang/ASTMatchers/ASTMatchers.h>

#include <queue>

using namespace clang::ast_matchers;

namespace jinja2
{
template<>
struct TypeReflection<codegen::Jinja2ReflectorGenerator::ReflectedStructInfo> : TypeReflected<codegen::Jinja2ReflectorGenerator::ReflectedStructInfo>
{
    static auto& GetAccessors()
    {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            {"name", [](const codegen::Jinja2ReflectorGenerator::ReflectedStructInfo& obj) {return obj.name;}},
            {"fullQualifiedName", [](const codegen::Jinja2ReflectorGenerator::ReflectedStructInfo& obj) {return obj.fullQualifiedName;}},
            {"members", [](const codegen::Jinja2ReflectorGenerator::ReflectedStructInfo& obj) {return Reflect(obj.members);}},
        };

        return accessors;
    }
};

template<>
struct TypeReflection<codegen::Jinja2ReflectorGenerator::ReflectedEnumInfo> : TypeReflected<codegen::Jinja2ReflectorGenerator::ReflectedEnumInfo>
{
    static auto& GetAccessors()
    {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            {"name", [](const codegen::Jinja2ReflectorGenerator::ReflectedEnumInfo& obj) {return obj.name;}},
            {"fullQualifiedName", [](const codegen::Jinja2ReflectorGenerator::ReflectedEnumInfo& obj) {return obj.fullQualifiedName;}},
            {"itemPrefix", [](const codegen::Jinja2ReflectorGenerator::ReflectedEnumInfo& obj) {return obj.itemPrefix;}},
            {"items", [](const codegen::Jinja2ReflectorGenerator::ReflectedEnumInfo& obj) {return Reflect(obj.items);}},
        };

        return accessors;
    }
};

template<>
struct TypeReflection<codegen::Jinja2ReflectorGenerator::MemberInfo> : TypeReflected<codegen::Jinja2ReflectorGenerator::MemberInfo>
{
    static auto& GetAccessors()
    {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            {"reflectedName", [](const codegen::Jinja2ReflectorGenerator::MemberInfo& obj) {return obj.reflectedName;}},
            {"accessString", [](const codegen::Jinja2ReflectorGenerator::MemberInfo& obj) {return obj.accessString;}},
        };

        return accessors;
    }
};
}

namespace codegen
{
namespace
{
DeclarationMatcher structsMatcher =
        cxxRecordDecl(isStruct(), isDefinition()).bind("struct");

auto g_template =
R"(
{% extends "header_skeleton.j2tpl" %}
{% block generator_headers %}
 #include <jinja2cpp/reflected_value.h>
{% endblock %}

{% block global_decls %}
namespace jinja2
{
namespace detail
{
{% for enum in reflectedEnums %}
template<>
struct Reflector<{{ enum.fullQualifiedName }}>
{
    static auto Create({{enum.fullQualifiedName}} val)
    {
        switch (val)
        {
        {% for i in enum.items %}
        case {{enum.itemPrefix}}{{i}}:
            return Value(std::string("{{i}}"));
        {% endfor %}
        }

        return Value();
    }
    static auto CreateFromPtr(const {{enum.fullQualifiedName}}* val)
    {
        return Create(*val);
    }
};
{% endfor %}
} // detail

{% for struct in reflectedStructs %}
template<>
struct TypeReflection<{{struct.fullQualifiedName}}> : TypeReflected<{{struct.fullQualifiedName}}>
{
    static auto& GetAccessors()
    {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
        {% for member in struct.members | sort(attribute='reflectedName') %}
            {"{{member.reflectedName}}", [](const {{struct.fullQualifiedName}}& obj) {
                return Reflect(obj.{{member.accessString}});
            } },
        {% endfor %}
        };

        return accessors;
    }
};
{% endfor %}
}
{% endblock %}
)";
}

Jinja2ReflectorGenerator::Jinja2ReflectorGenerator(const Options& opts)
    : BasicGenerator(opts)
{

}

void Jinja2ReflectorGenerator::SetupMatcher(clang::ast_matchers::MatchFinder& finder, clang::ast_matchers::MatchFinder::MatchCallback* defaultCallback)
{
    finder.addMatcher(structsMatcher, defaultCallback);

}

void Jinja2ReflectorGenerator::HandleMatch(const clang::ast_matchers::MatchFinder::MatchResult& matchResult)
{
    if (const clang::CXXRecordDecl* decl = matchResult.Nodes.getNodeAs<clang::CXXRecordDecl>("struct"))
    {
        if (!IsFromInputFiles(decl->getLocStart(), matchResult.Context))
            return;

        reflection::AstReflector reflector(matchResult.Context);

        auto structInfo = reflector.ReflectClass(decl, &m_namespaces);
        PrepareStructInfo(structInfo, matchResult.Context);
    }
}

bool Jinja2ReflectorGenerator::Validate()
{
    return true;
}

void Jinja2ReflectorGenerator::PrepareStructInfo(reflection::ClassInfoPtr info, clang::ASTContext* context)
{
    using namespace reflection;
    ReflectedStructInfo structInfo;
    structInfo.name = info->name;
    structInfo.fullQualifiedName = info->GetFullQualifiedName();

    std::queue<reflection::ClassInfoPtr> classesStack;
    std::set<std::string> reflectedClasses;

    classesStack.push(info);

    reflection::AstReflector reflector(context);

    while (!classesStack.empty())
    {
        reflection::ClassInfoPtr curInfo = classesStack.front();
        classesStack.pop();

        for (reflection::MemberInfoPtr& m : curInfo->members)
        {
            if (m->accessType == reflection::AccessType::Undefined || m->accessType == reflection::AccessType::Public)
            {
                MemberInfo memberInfo;
                memberInfo.reflectedName = m->name;
                memberInfo.accessString = m->name;
                const EnumType* et = m->type->getAsEnumType();
                if (et)
                    PrepareEnumInfo(reflector.ReflectEnum(et->decl, &m_namespaces), context);

                structInfo.members.push_back(std::move(memberInfo));
            }
        }

        for (reflection::MethodInfoPtr& m : curInfo->methods)
        {
            if (m->accessType == reflection::AccessType::Undefined || m->accessType == reflection::AccessType::Public)
            {
                if (m->params.size() != 0 || m->isCtor || m->isDtor || m->isStatic || m->isOperator || m->isImplicit)
                    continue;

                MemberInfo memberInfo;
                memberInfo.reflectedName = m->name.find("Get") == 0 ? m->name.substr(3) : m->name;
                memberInfo.accessString = m->name + "()";
                const EnumType* et = m->returnType->getAsEnumType();
                if (et)
                    PrepareEnumInfo(reflector.ReflectEnum(et->decl, &m_namespaces), context);

                structInfo.members.push_back(std::move(memberInfo));
            }
        }

        for (auto& b : curInfo->baseClasses)
        {
            if (b.accessType != AccessType::Undefined && b.accessType != AccessType::Public)
                continue;

            TypeInfoPtr baseType = b.baseClass;
            const RecordType* baseClassType = baseType->getAsRecord();
            if (!baseClassType)
                continue;

            reflection::ClassInfoPtr baseClassInfo = reflector.ReflectClass(llvm::dyn_cast_or_null<clang::CXXRecordDecl>(baseClassType->decl), &m_namespaces);
            if (reflectedClasses.count(baseClassInfo->GetFullQualifiedName()) == 1)
                continue;
            classesStack.push(baseClassInfo);
        }

        reflectedClasses.insert(curInfo->GetFullQualifiedName());
    }

    m_reflectedStructs.push_back(std::move(structInfo));
}

void Jinja2ReflectorGenerator::PrepareEnumInfo(reflection::EnumInfoPtr info, clang::ASTContext* context)
{
    std::string fullQualifiedName = info->GetFullQualifiedName();
    if (m_reflectedEnumsSet.count(fullQualifiedName))
        return;

    ReflectedEnumInfo enumInfo;
    enumInfo.name = info->name;
    enumInfo.fullQualifiedName = fullQualifiedName;
    enumInfo.itemPrefix = (info->isScoped ? fullQualifiedName : info->GetFullQualifiedScope(false)) + "::";

    for (auto& item : info->items)
        enumInfo.items.push_back(item.itemName);

    m_reflectedEnums.push_back(std::move(enumInfo));
    m_reflectedEnumsSet.insert(fullQualifiedName);
}

void Jinja2ReflectorGenerator::WriteHeaderPreamble(CppSourceStream& hdrOs)
{
    ;
}

void Jinja2ReflectorGenerator::WriteHeaderPostamble(CppSourceStream& hdrOs)
{

}

void Jinja2ReflectorGenerator::WriteHeaderContent(CppSourceStream& hdrOs)
{
    jinja2::Template tpl(&m_templateEnv);
    tpl.Load(g_template);

    jinja2::ValuesMap params = {
        {"inputFiles", jinja2::Reflect(m_options.inputFiles)},
        {"headerGuard", GetHeaderGuard(m_options.outputHeaderName)},
        {"reflectedStructs", jinja2::Reflect(m_reflectedStructs)},
        {"reflectedEnums", jinja2::Reflect(m_reflectedEnums)}
    };

    SetupCommonTemplateParams(params);

    tpl.Render(hdrOs, params);
}


} // codegen

codegen::GeneratorPtr CreateJinja2ReflectGen(const codegen::Options& opts)
{
    return std::make_unique<codegen::Jinja2ReflectorGenerator>(opts);
}


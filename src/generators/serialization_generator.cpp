#include "serialization_generator.h"
#include "options.h"
#include "ast_reflector.h"
#include "ast_utils.h"
#include "jinja2_reflection.h"

#include <clang/ASTMatchers/ASTMatchers.h>

#include <queue>

using namespace clang::ast_matchers;

namespace codegen
{
namespace
{
DeclarationMatcher structsMatcher =
        cxxRecordDecl(isStruct(), isDefinition()).bind("struct");

}

SerializationGenerator::SerializationGenerator(const Options& opts)
    : BasicGenerator(opts)
{

}

void SerializationGenerator::SetupMatcher(clang::ast_matchers::MatchFinder& finder, clang::ast_matchers::MatchFinder::MatchCallback* defaultCallback)
{
    finder.addMatcher(structsMatcher, defaultCallback);

}

void SerializationGenerator::HandleMatch(const clang::ast_matchers::MatchFinder::MatchResult& matchResult)
{
    if (const clang::CXXRecordDecl* decl = matchResult.Nodes.getNodeAs<clang::CXXRecordDecl>("struct"))
    {
        if (!IsFromInputFiles(decl->getBeginLoc(), matchResult.Context))
            return;

        reflection::AstReflector reflector(matchResult.Context, m_options.consoleWriter);

        auto structInfo = reflector.ReflectClass(decl, &m_namespaces);
        PrepareStructInfo(structInfo, matchResult.Context);
    }
}

bool SerializationGenerator::Validate()
{
    return true;
}

void SerializationGenerator::PrepareStructInfo(reflection::ClassInfoPtr info, clang::ASTContext* context)
{
    using namespace reflection;

    std::queue<reflection::ClassInfoPtr> classesStack;
    std::set<std::string> reflectedClasses;

    classesStack.push(info);

    reflection::AstReflector reflector(context, m_options.consoleWriter);

    while (!classesStack.empty())
    {
        reflection::ClassInfoPtr curInfo = classesStack.front();
        classesStack.pop();

        for (reflection::MemberInfoPtr& m : curInfo->members)
        {
            if (m->accessType == reflection::AccessType::Undefined || m->accessType == reflection::AccessType::Public)
            {
                const EnumType* et = m->type->getAsEnumType();
                if (et)
                    reflector.ReflectEnum(et->decl, &m_namespaces);

//                const RecordType* rt = m->type->getAsRecord();
//                if ()
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
}

void SerializationGenerator::PrepareEnumInfo(reflection::EnumInfoPtr info, clang::ASTContext* context)
{
#if 0
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
#endif
}

void SerializationGenerator::WriteHeaderPreamble(CppSourceStream& hdrOs)
{
    ;
}

void SerializationGenerator::WriteHeaderPostamble(CppSourceStream& hdrOs)
{

}

void SerializationGenerator::WriteHeaderContent(CppSourceStream& hdrOs)
{
    jinja2::ValuesMap params = {
        { "headerGuard", GetHeaderGuard(m_options.outputHeaderName) },
        { "rootNamespace", jinja2::Reflect(m_namespaces.GetRootNamespace()) }
    };

    RenderTemplate(hdrOs, params);
}


} // codegen

codegen::GeneratorPtr CreateSerializationGen(const codegen::Options& opts)
{
    return std::make_unique<codegen::SerializationGenerator>(opts);
}


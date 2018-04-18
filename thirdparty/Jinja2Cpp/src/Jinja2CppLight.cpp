// Copyright Hugh Perkins 2015 hughperkins at gmail
//
// This Source Code Form is subject to the terms of the Mozilla Public License,
// v. 2.0. If a copy of the MPL was not distributed with this file, You can
// obtain one at http://mozilla.org/MPL/2.0/.

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <sstream>

#include "stringhelper.h"

#include "Jinja2CppLight.h"

using namespace std;

namespace
{
    const std::string JINJA2_TRUE = "True";
    const std::string JINJA2_FALSE = "False";
    const std::string JINJA2_NOT = "not";
}

namespace Jinja2CppLight {

#undef VIRTUAL
#define VIRTUAL
#undef STATIC
#define STATIC


Value::Value() = default;

//Value::Value(ValueData data)
//    : m_data(std::move(data))
//{

//}

namespace
{
struct ValueRenderer : boost::static_visitor<std::string>
{
    std::string operator() (bool val) const
    {
        return val ? JINJA2_TRUE : JINJA2_FALSE;
    }

    std::string operator() (const ValuesList& vals) const
    {
        std::string result = "{";
        bool isFirst = true;
        for (auto& val : vals)
        {
            if (isFirst)
                isFirst = false;
            else
                result += ", ";

            result += boost::apply_visitor(ValueRenderer(), val.data());
        }
        result += "}";
        return result;
    }

    std::string operator() (const ValuesMap& vals) const
    {
        std::string result = "{";
        bool isFirst = true;
        for (auto& val : vals)
        {
            if (isFirst)
                isFirst = false;
            else
                result += ", ";

            result += "{\"" + val.first + "\",";
            result += boost::apply_visitor(ValueRenderer(), val.second.data());
            result += "}";
        }
        result += "}";
        return result;
    }

    template<typename T>
    std::string operator() (const T& val) const
    {
        return toString(val);
    }
};

struct ValueToBoolConverter : boost::static_visitor<bool>
{
    bool operator() (int64_t val) const
    {
        return val != 0;
    }
    bool operator() (double val) const
    {
        return fabs(val) < std::numeric_limits<double>::epsilon();
    }
    bool operator() (bool val) const
    {
        return val;
    }
    template<typename T>
    bool operator() (const T& val) const
    {
        return !val.empty();
    }
};
} //

string Value::render() const
{
    return boost::apply_visitor(ValueRenderer(), m_data);
}

bool Value::isTrue() const
{
    return boost::apply_visitor(ValueToBoolConverter(), m_data);
}

Template::Template( std::string sourceCode ) :
    sourceCode( sourceCode ) {
    root = std::make_shared<Root>();
//    cout << "template::Template root: "  << root << endl;
}

STATIC bool Template::isNumber( std::string astring, int *p_value ) {
    istringstream in( astring );
    int value;
    if( in >> value && in.eof() ) {
        *p_value = value;
        return true;
    }
    return false;
}
VIRTUAL Template::~Template() {
}

Template&Template::setValue(string name, Value value)
{
    valueByName[ std::move(name) ] = std::move(value);
    return *this;
}

Template&Template::setValues(const ValuesMap& values)
{
    valueByName = values;
    return *this;
}

void Template::removeValue(string name)
{
    valueByName.erase(name);
}
std::string Template::render() {
    size_t finalPos = eatSection(0, root.get() );
    if( finalPos != sourceCode.length() ) {
        throw render_error("some sourcecode found at end: " + sourceCode.substr( finalPos ) );
    }
    return root->render(valueByName);
}

void Template::print(ControlSection *section) {
    section->print("");
}

// pos should point to the first character that has sourcecode inside the control section controlSection
// return value should be first character of the control section end part (ie first char of {% endfor %} type bit)
int Template::eatSection( int pos, ControlSection *controlSection ) {
//    int pos = 0;
//    vector<string> tokenStack;
//    string updatedString = "";
    while( true ) {
//        cout << "pos: " << pos << endl;
        size_t controlChangeBegin = sourceCode.find( "{%", pos );
//        cout << "controlChangeBegin: " << controlChangeBegin << endl;
        if( controlChangeBegin == string::npos ) {
            //updatedString += doSubstitutions( sourceCode.substr( pos ), valueByName );
            Code *code = new Code();
            code->startPos = pos;
            code->endPos = sourceCode.length();
//            code->templateCode = sourceCode.substr( pos, sourceCode.length() - pos );
            code->templateCode = sourceCode.substr( code->startPos, code->endPos - code->startPos );
            controlSection->sections.push_back( code );
            return sourceCode.length();
        } else {
            size_t controlChangeEnd = sourceCode.find( "%}", controlChangeBegin );
            if( controlChangeEnd == string::npos ) {
                throw render_error( "control section unterminated: " + sourceCode.substr( controlChangeBegin, 40 ) );
            }
            string controlChange = trim( sourceCode.substr( controlChangeBegin + 2, controlChangeEnd - controlChangeBegin - 2 ) );
            vector<string> splitControlChange = split( controlChange, " " );
            if( splitControlChange[0] == "endfor" || splitControlChange[0] == "endif") {
                if( splitControlChange.size() != 1 ) {
                    throw render_error("control section {% " + controlChange + " unrecognized" );
                }
                Code *code = new Code();
                code->startPos = pos;
                code->endPos = controlChangeBegin;
                code->templateCode = sourceCode.substr( code->startPos, code->endPos - code->startPos );
                controlSection->sections.push_back( code );
                return controlChangeBegin;
//                if( tokenStack.size() == 0 ) {
//                    throw render_error("control section {% " + controlChange + " unexpected: no current control stack items" );
//                }
//                if( tokenStack[ tokenStack.size() - 1 ] != "for" ) {
//                    throw render_error("control section {% " + controlChange + " unexpected: current last control stack item is: " + tokenStack[ tokenStack.size() - 1 ] );
//                }
//                cout << "token stack old size: " << tokenStack.size() << endl;
//                tokenStack.erase( tokenStack.end() - 1, tokenStack.end() - 1 );
//                string varToRemove = varNameStack[ (int)tokenStack.size() - 1 ];
//                valueByName.erase( varToRemove );
//                varNameStack.erase( tokenStack.end() - 1, tokenStack.end() - 1 );
//                cout << "token stack new size: " << tokenStack.size() << endl;
            } else if( splitControlChange[0] == "for" ) {
                Code *code = new Code();
                code->startPos = pos;
                code->endPos = controlChangeBegin;
                code->templateCode = sourceCode.substr( code->startPos, code->endPos - code->startPos );
                controlSection->sections.push_back( code );

                auto vars = split(splitControlChange[1], ",");
                if( splitControlChange[2] != "in" ) {
                    throw render_error("control section {% " + controlChange + " unexpected: second word should be 'in'" );
                }

                string name = splitControlChange[3];
                ForSection *forSection = new ForSection();
                forSection->varNames = std::move(vars);
                forSection->valueVarName = std::move(name);
                pos = eatSection( controlChangeEnd + 2, forSection );
                controlSection->sections.push_back(forSection);
                size_t controlEndEndPos = sourceCode.find("%}", pos );
                if( controlEndEndPos == string::npos ) {
                    throw render_error("No control end section found at: " + sourceCode.substr(pos ) );
                }
                string controlEnd = sourceCode.substr( pos, controlEndEndPos - pos + 2 );
                string controlEndNorm = replaceGlobal( controlEnd, " ", "" );
                if( controlEndNorm != "{%endfor%}" ) {
                    throw render_error("No control end section found, expected '{% endfor %}', got '" + controlEnd + "'" );
                }
                forSection->endPos = controlEndEndPos + 2;
                pos = controlEndEndPos + 2;
//                tokenStack.push_back("for");
//                varNameStack.push_back(name);
            } else if (splitControlChange[0] == "if") {
                Code *code = new Code();
                code->startPos = pos;
                code->endPos = controlChangeBegin;
                code->templateCode = sourceCode.substr(code->startPos, code->endPos - code->startPos);
                controlSection->sections.push_back(code);
                const string word = splitControlChange[1];
                if (JINJA2_TRUE == word)  {
                    ;
                } else if (JINJA2_FALSE == word) {
                    ;
                }
                else if (JINJA2_NOT == word) {
                    ;
                }
                else {
                    ;
                }
                IfSection* ifSection = new IfSection(controlChange);

                pos = eatSection(controlChangeEnd + 2, ifSection);
                controlSection->sections.push_back(ifSection);
                size_t controlEndEndPos = sourceCode.find("%}", pos);
                if (controlEndEndPos == string::npos) {
                    throw render_error("No control end of any section found at: " + sourceCode.substr(pos));
                }
                string controlEnd = sourceCode.substr(pos, controlEndEndPos - pos + 2);
                string controlEndNorm = replaceGlobal(controlEnd, " ", "");
                if (controlEndNorm != "{%endif%}") {
                    throw render_error("No control end section found, expected '{% endif %}', got '" + controlEnd + "'");
                }
                //forSection->endPos = controlEndEndPos + 2;
                pos = controlEndEndPos + 2;

            } else {
                throw render_error("control section {% " + controlChange + " unexpected" );
            }
        }
    }

//    vector<string> controlSplit = split( sourceCode, "{%" );
////    int startI = 1;
////    if( controlSplit.substr(0,2) == "{%" ) {
////        startI = 0;
////    }
//    string updatedString = "";
//    for( int i = 0; i < (int)controlSplit.size(); i++ ) {
//        if( controlSplit[i].substr(0,2) == "{%" ) {
//            vector<string> splitControlPair = split(controlSplit[i], "%}" );
//            string controlString = splitControlPair[0];
//        } else {
//            updatedString += doSubstitutions( controlSplit[i], valueByName );
//        }
//    }
////    string templatedString = doSubstitutions( sourceCode, valueByName );
//    return updatedString;
}
STATIC std::string Template::doSubstitutions( const std::string &sourceCode, const ValuesMap &valueByName ) {
    int startI = 1;
    if( sourceCode.substr(0,2) == "{{" ) {
        startI = 0;
    }
    string templatedString = "";
    vector<string> splitSource = split( sourceCode, "{{" );
    if( startI == 1 ) {
        templatedString = splitSource[0];
    }
    for( size_t i = startI; i < splitSource.size(); i++ ) {
        if (0 == i && splitSource.size() > 1 && splitSource[i].empty()) // Ignoring initial empty section if there are other sections.
        {
            continue;
        }

        vector<string> thisSplit = split( splitSource[i], "}}" );
        string namesStr = trim( thisSplit[0] );
        auto names = split(namesStr, ".");

        const ValuesMap* curValues = &valueByName;
        const Value* curValue = nullptr;
        for (auto& name : names)
        {
            if (curValue != nullptr)
            {
                if (!curValue->isMap())
                {
                    curValue = nullptr;
                    break;
                }
                curValues = &curValue->asMap();
            }
            auto p = curValues->find(name);

            if (p == curValues->end() )
            {
                curValue = nullptr;
                break;
            }
            curValue = &p->second;
        }

        if (curValue == nullptr)
            templatedString += "<<## variable " + namesStr + " not accessible ##>>";
        else
            templatedString += curValue->render();

        if( thisSplit.size() > 0 ) {
            templatedString += thisSplit[1];
        }
    }
    return templatedString;
}

void IfSection::parseIfCondition(const std::string& expression) {
    const std::vector<std::string> splittedExpression = split(expression, " ");
    if (splittedExpression.empty() || splittedExpression[0] != "if") {
        throw render_error("if statement expected.");
    }

    std::size_t expressionIndex = 1;
    if (splittedExpression.size() < expressionIndex + 1) {
        throw render_error("Any expression expected after if statement.");
    }
    m_isNegation = (JINJA2_NOT == splittedExpression[expressionIndex]);
    expressionIndex += (m_isNegation) ? 1 : 0;
    if (splittedExpression.size() < expressionIndex + 1) {
        if (!m_isNegation)
            throw render_error("Any expression expected after if statement.");
        else
            throw render_error("Any expression expected after if not statement.");
    }
    m_variableName = splittedExpression[expressionIndex];
    if (splittedExpression.size() > expressionIndex + 1) {
        throw render_error(std::string("Unexpected expression after variable name: ") + splittedExpression[expressionIndex + 1]);
    }
}

bool IfSection::computeExpression(const ValuesMap &valueByName) const {
    if (JINJA2_TRUE == m_variableName) {
        return true ^ m_isNegation;
    }
    else if (JINJA2_FALSE == m_variableName) {
        return false ^ m_isNegation;
    }
    else {
        const bool valueExists = valueByName.count(m_variableName) > 0;
        if (!valueExists) {
            return false ^ m_isNegation;
        }
        return valueByName.at(m_variableName).isTrue() ^ m_isNegation;
    }
}

string ForSection::render(ValuesMap& valueByName) {
    std::string result = "";
    auto oldVarNames = varNames;
    oldVarNames.emplace_back("loop");
    //        bool nameExistsBefore = false;

    ValuesMap oldValues;
    for (auto& name : oldVarNames)
    {
        auto p = valueByName.find(name);
        if (p != valueByName.end())
            oldValues[name] = std::move(p->second);
    }

    valueByName["loop"] = ValuesMap();
    auto& loopVar = valueByName["loop"].asMap();
    ValuesList* loopItems = nullptr;
    ValuesList fakeItems;

    auto p = valueByName.find(valueVarName);
    if (p == valueByName.end() || !p->second.isList())
        loopItems = &fakeItems;
    else
        loopItems = &p->second.asList();

    int64_t itemsNum = static_cast<int64_t>(loopItems->size());
    loopVar["length"] = Value(itemsNum);
    for (int64_t itemIdx = 0; itemIdx != itemsNum; ++ itemIdx)
    {
        loopVar["index"] = Value(itemIdx + 1);
        loopVar["index0"] = Value(itemIdx);
        loopVar["first"] = Value(itemIdx == 0);
        loopVar["last"] = Value(itemIdx == itemsNum - 1);
        if (itemIdx != 0)
            loopVar["previtem"] = (*loopItems)[itemIdx - 1];
        if (itemIdx != itemsNum - 1)
            loopVar["nextitem"] = (*loopItems)[itemIdx + 1];
        else
            loopVar.erase("nextitem");

        auto& curValue = (*loopItems)[itemIdx];
        if (varNames.size() > 1 && curValue.isMap())
        {
            auto& valTuple = curValue.asMap();
            for (auto& varName : varNames)
            {
                auto p = valTuple.find(varName);
                if (p != valTuple.end())
                    valueByName[varName] = p->second;
                else
                    valueByName.erase(varName);
            }
        }
        else
            valueByName[varNames[0]] = curValue;

        for( size_t j = 0; j < sections.size(); j++ ) {
            result += sections[j]->render( valueByName );
        }
    }


    for (auto& name : oldVarNames)
    {
        auto p = oldValues.find(name);
        if (p != oldValues.end())
            valueByName[name] = std::move(p->second);
        else
            valueByName.erase(name);
    }

    return result;
}



}



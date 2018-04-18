// Copyright Hugh Perkins 2015 hughperkins at gmail
//
// This Source Code Form is subject to the terms of the Mozilla Public License,
// v. 2.0. If a copy of the MPL was not distributed with this file, You can
// obtain one at http://mozilla.org/MPL/2.0/.

// the intent here is to create a templates library that:
// - is based on Jinja2 syntax
// - doesn't depend on boost, qt, etc ...

// for now, will handle:
// - variable substitution, ie {{myvar}}
// - for loops, ie {% for i in range(myvar) %}

#pragma once

#include <string>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <sstream>
#include "stringhelper.h"

#include <boost/variant.hpp>

#define VIRTUAL virtual
#define STATIC static

namespace Jinja2CppLight {

class render_error : public std::runtime_error {
public:
    render_error( const std::string &what ) :
        std::runtime_error( what ) {
    }
};

class Value;
using ValuesList = std::vector<Value>;
using ValuesMap = std::unordered_map<std::string, Value>;
using ValueData = boost::variant<bool, std::string, int64_t, double, ValuesList, ValuesMap>;

class Value {
public:
    Value();
    template<typename T>
    Value(T&& val, typename std::enable_if<!std::is_same<std::decay_t<T>, Value>::value>::type* = nullptr)
        : m_data(std::forward<T>(val))
    {
    }
    Value(const char* val)
        : m_data(std::string(val))
    {
    }
    template<size_t N>
    Value(char (&val)[N])
        : m_data(std::string(val))
    {
    }
    Value(int val)
        : m_data(static_cast<int64_t>(val))
    {
    }

    std::string render() const;
    bool isTrue() const;

    const ValueData& data() const {return m_data;}

    bool isList() const
    {
        return boost::get<ValuesList>(&m_data) != nullptr;
    }
    auto& asList()
    {
        return boost::get<ValuesList>(m_data);
    }
    auto& asList() const
    {
        return boost::get<ValuesList>(m_data);
    }
    bool isMap() const
    {
        return boost::get<ValuesMap>(&m_data) != nullptr;
    }
    auto& asMap()
    {
        return boost::get<ValuesMap>(m_data);
    }
    auto& asMap() const
    {
        return boost::get<ValuesMap>(m_data);
    }

private:
    ValueData m_data;
};

class Root;
class ControlSection;

class Template {
public:
    std::string sourceCode;

    ValuesMap valueByName;
    std::shared_ptr<Root> root;

    // [[[cog
    // import cog_addheaders
    // cog_addheaders.add(classname='Template')
    // ]]]
    // generated, using cog:
    Template( std::string sourceCode );
    STATIC bool isNumber( std::string astring, int *p_value );
    VIRTUAL ~Template();
    template<typename T>
    Template &setValue(std::string name, T&& val)
    {
        return setValue(std::move(name), Value(std::forward<T>(val)));
    }
    Template &setValue( std::string name, Value value );
    Template &setValues(const ValuesMap& values);
    void removeValue(std::string name);
    std::string render();
    void print(ControlSection *section);
    int eatSection( int pos, ControlSection *controlSection );
    STATIC std::string doSubstitutions( const std::string &sourceCode, const ValuesMap &valueByName );

    // [[[end]]]
};

class ControlSection {
public:
    std::vector< ControlSection * >sections;
    virtual std::string render( ValuesMap &valueByName ) = 0;
    virtual void print() {
        print("");
    }
    virtual void print(std::string prefix) = 0;
};

class Container : public ControlSection {
public:
//    std::vector< ControlSection * >sections;
    int sourceCodePosStart;
    int sourceCodePosEnd;

//    std::string render( std::map< std::string, Value *> valueByName );
    virtual void print( std::string prefix ) {
        std::cout << prefix << "Container ( " << sourceCodePosStart << ", " << sourceCodePosEnd << " ) {" << std::endl;
        for( int i = 0; i < (int)sections.size(); i++ ) {
            sections[i]->print( prefix + "    " );
        }
        std::cout << prefix << "}" << std::endl;
    }
};

class ForSection : public ControlSection {
public:
    std::vector<std::string> varNames;
    std::string valueVarName;
    int startPos;
    int endPos;

    std::string render( ValuesMap &valueByName ) override;
    //Container *contents;
    virtual void print( std::string prefix ) {
        std::cout << prefix << "For ( " << varNames[0] << " in " << valueVarName << " ) {" << std::endl;
        for( int i = 0; i < (int)sections.size(); i++ ) {
            sections[i]->print( prefix + "    " );
        }
        std::cout << prefix << "}" << std::endl;
    }
};

class Code : public ControlSection {
public:
//    vector< ControlSection * >sections;
    int startPos;
    int endPos;
    std::string templateCode;

    std::string render();
    virtual void print( std::string prefix ) {
        std::cout << prefix << "Code ( " << startPos << ", " << endPos << " ) {" << std::endl;
        for( int i = 0; i < (int)sections.size(); i++ ) {
            sections[i]->print( prefix + "    " );
        }
        std::cout << prefix << "}" << std::endl;
    }
    virtual std::string render( ValuesMap &valueByName ) override {
//        std::string templateString = sourceCode.substr( startPos, endPos - startPos );
//        std::cout << "Code section, rendering [" << templateCode << "]" << std::endl;
        std::string processed = Template::doSubstitutions( templateCode, valueByName );
//        std::cout << "Code section, after rendering: [" << processed << "]" << std::endl;
        return processed;
    }
};

class Root : public ControlSection {
public:
    virtual ~Root() {}
    virtual std::string render( ValuesMap &valueByName ) override {
        std::string resultString = "";
        for( int i = 0; i < (int)sections.size(); i++ ) {
            resultString += sections[i]->render( valueByName );
        }
        return resultString;
    }
    virtual void print(std::string prefix) {
        std::cout << prefix << "Root {" << std::endl;
        for( int i = 0; i < (int)sections.size(); i++ ) {
            sections[i]->print( prefix + "    " );
        }
        std::cout << prefix << "}" << std::endl;
    }
};

class IfSection : public ControlSection {
public:
    IfSection(const std::string& expression) {
        parseIfCondition(expression);
    }

    std::string render(ValuesMap &valueByName) override {
        std::stringstream ss;
        const bool expressionValue = computeExpression(valueByName);
        if (expressionValue) {
            for (size_t j = 0; j < sections.size(); j++) {
                ss << sections[j]->render(valueByName);
            }
        }
        const std::string renderResult = ss.str();
        return renderResult;
    }

    void print(std::string prefix) {
        std::cout << prefix << "if ( "
            << ((m_isNegation) ? "not " : "")
            << m_variableName << " ) {" << std::endl;
        if (true) {
            for (int i = 0; i < (int)sections.size(); i++) {
                sections[i]->print(prefix + "    ");
            }
        }
        std::cout << prefix << "}" << std::endl;
    }

private:
    //? It determines m_isNegation and m_variableName from @param[in] expression.
    //? @param[in] expression E.g. "if not myVariable" where myVariable is set by myTemplate.setValue( "myVariable", <any_value> );
    //?                       The result of this statement is false if myVariable is initialized.
    void parseIfCondition(const std::string& expression);

    bool computeExpression(const ValuesMap& valueByName) const;

    bool m_isNegation; ///< Tells whether is there "if not" or just "if" at the begin of expression.
    std::string m_variableName; ///< This simple "if" implementation allows single variable condition only.
};

}


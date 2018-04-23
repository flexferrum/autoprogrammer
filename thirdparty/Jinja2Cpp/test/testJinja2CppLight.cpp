// Copyright Hugh Perkins 2015 hughperkins at gmail
//
// This Source Code Form is subject to the terms of the Mozilla Public License,
// v. 2.0. If a copy of the MPL was not distributed with this file, You can
// obtain one at http://mozilla.org/MPL/2.0/.

#include <iostream>
#include <string>

#include "gtest/gtest.h"

#include "template.h"

using namespace std;
using namespace jinja2;

#if 0
TEST(testSpeedTemplates, ifTrueTest) {
    const std::string source = "abc{% if True %}def{% endif %}ghi";
    Template mytemplate( source );
    const string result = mytemplate.render();
    std::cout << "[" << result << "]" << endl;
    const std::string expectedResult = "abcdefghi";

    EXPECT_EQ(expectedResult, result);
}

TEST(testSpeedTemplates, ifFalseTest) {
    const std::string source = "abc{% if False %}def{% endif %}ghi";
    Template mytemplate(source);
    const string result = mytemplate.render();
    std::cout << "[" << result << "]" << endl;
    const std::string expectedResult = "abcghi";

    EXPECT_EQ(expectedResult, result);
}

TEST(testSpeedTemplates, ifNotTrueTest) {
    const std::string source = "abc{% if not True %}def{% endif %}ghi";
    Template mytemplate(source);
    const string result = mytemplate.render();
    std::cout << "[" << result << "]" << endl;
    const std::string expectedResult = "abcghi";

    EXPECT_EQ(expectedResult, result);
}

TEST(testSpeedTemplates, ifNotFalseTest) {
    const std::string source = "abc{% if not False %}def{% endif %}ghi";
    Template mytemplate(source);
    const string result = mytemplate.render();
    std::cout << "[" << result << "]" << endl;
    const std::string expectedResult = "abcdefghi";

    EXPECT_EQ(expectedResult, result);
}

TEST(testSpeedTemplates, ifVariableExitsTest) {
    const std::string source = "abc{% if its %}def{% endif %}ghi";

    {
        Template mytemplate(source);
        const std::string expectedResultNoVariable = "abcghi";
        const std::string result = mytemplate.render();
        EXPECT_EQ(expectedResultNoVariable, result);
    }

    {
        Template mytemplate(source);
        mytemplate.setValue("its", 3);
        const std::string result = mytemplate.render();
        std::cout << "[" << result << "]" << endl;
        const std::string expectedResult = "abcdefghi";
        EXPECT_EQ(expectedResult, result);
    }
}

TEST(testSpeedTemplates, ifVariableDoesntExitTest) {
    const std::string source = "abc{% if not its %}def{% endif %}ghi";

    {
        Template mytemplate(source);
        const std::string expectedResultNoVariable = "abcdefghi";
        const std::string result = mytemplate.render();
        EXPECT_EQ(expectedResultNoVariable, result);
    }

    {
        Template mytemplate(source);
        mytemplate.setValue("its", 3);
        const std::string result = mytemplate.render();
        std::cout << "[" << result << "]" << endl;
        const std::string expectedResult = "abcghi";
        EXPECT_EQ(expectedResult, result);
    }
}

TEST(testSpeedTemplates, ifUnexpectedExpression) {
    const std::string source = "abc{% if its is defined %}def{% endif %}ghi";
    Template myTemplate(source);
    bool threw = false;
    try {
        myTemplate.render();
    }
    catch (const render_error &e) {
        EXPECT_EQ(std::string("Unexpected expression after variable name: is"), e.what());
        threw = true;
    }
    EXPECT_EQ(true, threw);
}
#endif


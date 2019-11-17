#include "test_structs.h"

#include <generated/json_serialization.h>
#include <gtest/gtest.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>

TEST(JsonSerialization, SimpleSerialization) 
{
    SimpleStruct s;
    s.intField = 100500;
    s.strField = "Hello World!";
    s.enumField = Enum::Item1;

    rapidjson::Document doc;

    JsonSerialize(doc, s, doc.GetAllocator());

    EXPECT_EQ(s.intField, doc["intField"]);
    EXPECT_EQ(s.strField.c_str(), doc["strField"]);
    EXPECT_STREQ("Item1", doc["enumField"].GetString());
}


int main(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

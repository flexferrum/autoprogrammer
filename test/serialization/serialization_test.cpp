#include "test_structs.h"
#include "hana_test_structs.h"

#include <generated/json_serialization.h>
#include <generated/protobuf_serialization.h>
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

    EXPECT_EQ(s.intField, doc["intField"].GetInt());
    EXPECT_STREQ(s.strField.c_str(), doc["strField"].GetString());
    EXPECT_STREQ("Item1", doc["enumField"].GetString());
}

TEST(JsonSerialization, SimpleSerializationDeserialization)
{
    SimpleStruct s;
    s.intField = 100500;
    s.strField = "Hello World!";
    s.enumField = Enum::Item1;

    rapidjson::Document doc;

    JsonSerialize(doc, s, doc.GetAllocator());
    SimpleStruct s2;
    JsonDeserialize(doc, s2);

    EXPECT_EQ(s.intField, s2.intField);
    EXPECT_EQ(s.strField, s2.strField);
    EXPECT_EQ(s.enumField, s2.enumField);
}

TEST(ProtobufSerialization, SimpleSerializationDeserialization)
{
    SimpleStruct s;
    s.intField = 100500;
    s.strField = "Hello World!";
    s.enumField = Enum::Item1;

    std::string buffer;
    {
        std::ostringstream os;
        SerializeToStream(s, os);
        buffer = os.str();
    }

    SimpleStruct s2;
    { 
        std::istringstream is(buffer);
        DeserializeFromStream(is, s2);
    }

    EXPECT_EQ(s.intField, s2.intField);
    EXPECT_EQ(s.strField, s2.strField);
    EXPECT_EQ(s.enumField, s2.enumField);
}

TEST(HanaSerialization, SimpleSerializationDeserialization)
{
    test_hana::SimpleStruct s;
    s.intField = 100500;
    s.strField = "Hello";

    std::string buffer;
    {
        std::ostringstream os;
        test_hana::SerializeToStream(s, os);
        buffer = os.str();
    }

    test_hana::SimpleStruct s2;
    {
        std::istringstream is(buffer);
        test_hana::DeserializeFromStream(is, s2);
    }

    EXPECT_EQ(s.intField, s2.intField);
    EXPECT_EQ(s.strField, s2.strField);
}

int main(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

#include <gtest/gtest.h>

#include <generated/enum_conv_gen.h>
#include "test_enums.h"

TEST(Enum2String, ConvertToString_Successfull)
{
    EXPECT_STREQ("Item1", Enum1ToString(Item1));
    EXPECT_STREQ("Item2", Enum1ToString(Item2));
    EXPECT_STREQ("Item3", Enum1ToString(Item3));

    EXPECT_STREQ("Item1", Enum2ToString(Enum2::Item1));
    EXPECT_STREQ("Item2", Enum2ToString(Enum2::Item2));
    EXPECT_STREQ("Item3", Enum2ToString(Enum2::Item3));
}

TEST(Enum2String, ConvertToString_Failed)
{
    EXPECT_STREQ("Unknown Item", Enum1ToString(static_cast<Enum1>(123)));
}

TEST(Enum2String, ConvertFromString_Successfull)
{
    EXPECT_EQ(Item1, StringToEnum1("Item1"));
    EXPECT_EQ(Item2, StringToEnum1("Item2"));
    EXPECT_EQ(Item3, StringToEnum1("Item3"));

    EXPECT_EQ(Enum2::Item1, StringToEnum2("Item1"));
    EXPECT_EQ(Enum2::Item2, StringToEnum2("Item2"));
    EXPECT_EQ(Enum2::Item3, StringToEnum2("Item3"));
}

TEST(Enum2String, ConvertFromString_Failed)
{
    EXPECT_THROW(StringToEnum2("Unspecified Item"), flex_lib::bad_enum_name);
}

int main(int argc, char* argv[])
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}


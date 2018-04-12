#include <gtest/gtest.h>
#include <flex_lib/detector.h>

#include "test_pimpl.h"

#include <array>
#include <iterator>

// #include <generated/pimpl_gen.h>

TEST(PimplGen, StrConstructor_Successfull)
{
    TestPimpl pimpl("1234");
    
    EXPECT_EQ("1234", pimpl.GetString());
}

TEST(PimplGen, IntConstructor_Successfull)
{
    TestPimpl pimpl(1234);
    
    EXPECT_EQ(1234, pimpl.GetNumber());
}

TEST(PimplGen, ArraySet_Successfull)
{
    unsigned numbers[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    TestPimpl pimpl(1234);
    
    pimpl.SetGeneratedValues(numbers);
    
    auto vals = pimpl.GetGeneratedValues();
    auto srcBeg = numbers;
    auto srcEnd = numbers + 10;
    auto dstBeg = vals.begin();
    auto dstEnd = vals.end();
    
    auto itPair = std::mismatch(srcBeg, srcEnd, dstBeg, dstEnd);
    EXPECT_TRUE(itPair.first == srcEnd);
    EXPECT_TRUE(itPair.second == dstEnd);
}

TEST(PimplGen, MoveOperations_Successfull)
{
    TestMoveable moveable;
    auto srcObjPtr = moveable.GetObject();
    
    TestPimpl pimpl(std::move(moveable));
    
    auto destObjPtr = pimpl.GetMoveablePtr();
    
    EXPECT_EQ(*srcObjPtr, *destObjPtr);
    EXPECT_EQ(srcObjPtr, destObjPtr);
}

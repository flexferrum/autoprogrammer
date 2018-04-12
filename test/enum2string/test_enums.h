#ifndef TEST_ENUMS_H
#define TEST_ENUMS_H

enum Enum1
{
    Item1,
    Item2,
    Item3
};

enum class Enum2
{
    Item3,
    Item2,
    Item1
};
namespace
{
enum TestAnonEnum1
{
    ItemAn1,
    ItemAn2,
    ItemAn3
};
}

namespace TestNs1 
{
enum TestNs1Enum1
{
    Item1,
    Item2,
    Item3
};

namespace
{
enum TestNs1AnonEnum1
{
    ItemAn1,
    ItemAn2,
    ItemAn3
};
}

inline namespace hidden
{
enum TestNs1HiddenEnum1
{
    ItemInl1,
    ItemInl2,
    ItemInl3
};
}

struct Struct
{
    enum TestNs1StructEnum1
    {
        Item1,
        Item2,
        Item3
    };    
};
} // TestNs1

#endif // TEST_ENUMS_H

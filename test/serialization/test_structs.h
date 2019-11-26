#ifndef TEST_STRUCTS_H
#define TEST_STRUCTS_H

#include <string>
#include <vector>
#include <set>

enum class Enum
{
	Item1,
	Item2
};

struct SimpleStruct
{
    int intField;
    std::string strField;
    Enum enumField;
};

struct ComplexStruct
{
    int intField;
    SimpleStruct innerStruct;
    std::vector<SimpleStruct> structVector;
    std::set<int> intSet;
};

#endif // TEST_STRUCTS_H

#pragma once

#include <boost/hana/define_struct.hpp>
#include <boost/hana/for_each.hpp>
#include <boost/hana/members.hpp>
#include <boost/hana/keys.hpp>
#include <boost/hana/at_key.hpp>
#include <string>
#include <iostream>

namespace hana = boost::hana;

namespace test_hana
{
struct SimpleStruct
{
    BOOST_HANA_DEFINE_STRUCT(SimpleStruct,
    (int, intField),
    (std::string, strField)
    );
};

template<typename T>
void SerializeToStream(const T& value, std::ostream& os)
{
    hana::for_each(hana::members(value), [&](auto member) {os << member << std::endl;});
}

template<typename T>
void DeserializeFromStream(std::istream& is, T& value)
{
    hana::for_each(hana::keys(value), [&](auto key) 
    {
        auto& member = hana::at_key(value, key);
        is >> member;
    });
}
}

#if 0

struct [[serializable]] SimpleStruct
{
    int intField;
    std::string strField;
    Enum enumField;
};

template<typename Class>
auto JsonSerialize(rapidjson::Value& node,
                   Class&& structValue,
                   rapidjson::Document::AllocatorType& allocator) -> std::enable_if_t<
        tinyrefl::has_metadata<std::decay_t<Class>>() &&
        tinyrefl::has_attribute<std::decay_t<Class>>("serializable")>
{
    tinyrefl::visit_member_variables(object, [&node, &allocator](const auto& name, const auto& var) {
        rapidjson::Value innerNode;
        JsonSerialize(innerNode, var, allocator);
        node.AddMember(name.c_str(), innerNode.Move(), allocator);
    });
    return equal;
}
#endif

// #include <boost/

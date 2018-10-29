#ifndef BOOST_SERIALIZATION_H
#define BOOST_SERIALIZATION_H

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/split_member.hpp>

#include <meta/metaclass.h>

$_metaclass(BoostSerializable)
{
    static void GenerateDecl()
    {
        $_inject(public) [&, name="serialize"](const auto& ar, unsigned int ver) -> void
        {
            $_constexpr for (auto& v : $BoostSerializable.variables())
                $_inject(_) ar & $_v(v.name());
        };
    }
};

$_metaclass(BoostSerializableSplitted)
{
    static void GenerateDecl()
    {
        $_inject(public) [&, name="load"](const auto& ar, unsigned int ver) -> void
        {
            $_constexpr for (auto& v : $BoostSerializableSplitted.variables())
                $_inject(_) ar << $_v(v.name());
        };

        $_inject(public) [&, name="save"](const auto& ar, unsigned int ver) -> void
        {
            $_constexpr for (auto& v : $BoostSerializableSplitted.variables())
                $_inject(_) ar >> $_v(v.name());
        };

        $_inject(public) [&, name="serialize"](auto& ar, const unsigned int ver) -> void
        {
            boost::serialization::split_member(ar, $_str(*this), ver);
        };
    }
};

$_struct(TestStruct, BoostSerializable)
{
    int a;
    std::string b;
};

$_struct(TestStruct1, BoostSerializableSplitted)
{
    int a;
    std::string b;
};

#endif // BOOST_SERIALIZATION_H

#ifndef BOOST_SERIALIZATION_H
#define BOOST_SERIALIZATION_H

// #include <boost/serialization/serialization.hpp>

#include <meta/metaclass.h>

$_metaclass(BoostSerializable)
{
    static void GenerateDecl()
    {
        $_inject(public) [&, name="serialize"](auto ar, unsigned int ver) -> void
        {
            ar.set_type(meta::template_type("Archive"));

            $_constexpr for (auto& v : $BoostSerializable.variables())
                $_inject(_) ar & $_v(v);
        };
    }
};

$_struct(TestStruct, BoostSerializable)
{
    int a;
    std::string b;
};

#endif // BOOST_SERIALIZATION_H

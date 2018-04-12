#ifndef UTILS_H
#define UTILS_H

#include <llvm/ADT/APSInt.h>
#include <string>
#include <type_traits>

struct IntegerValue
{
    bool isSigned = false;
    union
    {
        uint64_t uintValue;
        int64_t intValue;
    };
    
    uint64_t AsUnsigned() const {return uintValue;}
    int64_t AsSigned() const {return intValue;}
    
    template<typename T>
    auto GetAs() -> std::enable_if_t<std::is_signed<T>::value, T>
    {
        return static_cast<T>(intValue);
    }
    
    template<typename T>
    auto GetAs() -> std::enable_if_t<std::is_unsigned<T>::value, T>
    {
        return static_cast<T>(uintValue);
    }
};

inline IntegerValue ConvertAPSInt(llvm::APSInt intValue)
{
    IntegerValue result;
    result.isSigned = intValue.isSigned();
    result.intValue = intValue.getExtValue();
    
    return result;
}

inline IntegerValue ConvertAPInt(llvm::APInt intValue)
{
    IntegerValue result;
    result.isSigned = false;
    result.uintValue = intValue.getZExtValue();
    
    return result;
}

template<typename Seq, typename Fn>
void WriteSeq(std::ostream& os, Seq&& seq, std::string delim, Fn&& ftor)
{
    bool isFirst = true;
    for (auto& i : seq)
    {
        if (isFirst)
            isFirst = false;
        else
            os << delim;
        
        ftor(os, i);
    }
}

template<typename Seq>
void WriteSeq(std::ostream& os, Seq&& seq, std::string delim = ", ")
{
    WriteSeq(os, std::forward<Seq>(seq), std::move(delim), [](auto&& os, auto&& i) {os << i;});
}

#endif // UTILS_H

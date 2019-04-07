#ifndef REFLECTED_RANGE_H
#define REFLECTED_RANGE_H

#include "reflected_object.h"
#include "value.h"

namespace codegen
{
namespace interpreter
{

template<typename It>
class StdCollectionIterator : public IteratorT, public std::enable_shared_from_this<StdCollectionIterator<It>>
{
public:
    using ThisType = StdCollectionIterator<It>;

    StdCollectionIterator(It it)
        : m_it(it)
    {}

    bool IsEqual(const IteratorT *other) const override
    {
        const ThisType* right = static_cast<const ThisType*>(other);
        return m_it == right->m_it;
    }

    Value GetValue() const override
    {
        auto val = *m_it;
        return Value(val);
    }

    void PrefixInc() override
    {
        ++ m_it;
    }

private:
    It m_it;
};

template<typename It>
IteratorTPtr MakeStdCollectionIterator(It&& it)
{
    return std::make_shared<StdCollectionIterator<It>>(std::forward<It>(it));
}

template<typename Coll>
class StdCollectionRefRange : public RangeT
{
public:
    StdCollectionRefRange(Coll coll)
        : m_coll(coll)
    {}

    // RangeT interface
    bool Empty() override
    {
        std::cout << "StdCollectionRefRange::Empty. Coll=" << &m_coll << std::endl;
        return m_coll.empty();
    }

    Value Begin() override
    {
        std::cout << "StdCollectionRefRange::Begin. Coll=" << &m_coll << std::endl;
        return ReflectedObject(MakeStdCollectionIterator(m_coll.begin()));
    }
    Value End() override
    {
        std::cout << "StdCollectionRefRange::End. Coll=" << &m_coll << std::endl;
        return ReflectedObject(MakeStdCollectionIterator(m_coll.end()));
    }

    Value ConstBegin() override {return Value();}
    Value ConstEnd() override {return Value();}

private:
    Coll m_coll;
};

template<typename Coll>
auto MakeStdCollectionRefRange(Coll&& coll)
{
    return std::make_shared<StdCollectionRefRange<Coll>>(std::forward<Coll>(coll));
}

}
}

#endif // REFLECTED_RANGE_H

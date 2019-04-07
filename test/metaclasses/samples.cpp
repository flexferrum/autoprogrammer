#if 0
#define interface struct
#define $class class

struct IUnknown
{
};

typedef bool BOOL;

struct IEnumerator : public IUnknown
{
    virtual ~IEnumerator() {}

    virtual HRESULT Reset() = 0;
    virtual HRESULT MoveNext(BOOL* succeeded) = 0;
    virtual HRESULT Current(IUnknown** item) = 0;
};

interface IEnumerator
{
    void Reset();
    BOOL MoveNext();
    IUnknown* Current();
};

#undef interface

$class interface
{
    virtual ~interface() {}
    constexpr
    {
        compiler.require($interface.variables().empty(), "interfaces may not contain data");
        for (auto f : $interface.functions())
        {
            compiler.require(!f.is_copy() && !f.is_move(),
                             "interfaces may not copy or move; consider a virtual clone() instead");
            if (!f.has_access())
                f.make_public();

            compiler.require(f.is_public(), "interface functions must be public");

            f.make_pure_virtual();
            f.parameters().add($f.return_type());
            f.set_type(HRESULT);
            f.bases().add(IUnknown);
        }
    }
};

template<typename T, typename S>
constexpr void interface(T target, const S source) {
    compiler.require(source.variables().empty(), "interfaces may not contain data");
    for (auto f : source.member_functions()) {
        compiler.require(!f.is_copy() && !f.is_move(),
                         "interfaces may not copy or move; consider a virtual clone() instead");
        if (!f.has_access())
            f.make_public();

        compiler.require(f.is_public(), "interface functions must be public");

        auto ret = f.return_type();
        if (ret == $void) {
            __extend(target) class {
                virtual HRESULT idexpr(f)(__inject(f.parameters())) = 0;
            };
        }
        else {
            __extend(target) class {
                virtual HRESULT idexpr(f)(__inject(f.parameters()), typename(ret)* retval) = 0;
            };
        }
    }
}

constexpr void basic_enum(meta::type target, const meta::type source) {
    value(target, source); // a basic_enum is-a value
    compiler.require(source.variables().size() > 0, "an enum cannot be empty");
    if (src.variables().front().type().is_auto())
        ->(target) { using U = int; } // underlying type
        else ->(target) { using U = (src.variables().front().type())$; }
        for (auto o : source.variables()) {
            if (!o.has_access()) o.make_public();
            if (!o.has_storage()) o.make_constexpr();
            if (o.has_auto_type()) o.set_type(U);
            compiler.require(o.is_public(), "enumerators must be public");
            compiler.require(o.is_constexpr(), "enumerators must be constexpr");
            compiler.require(o.type() == U, "enumerators must use same type");
            ->(target) o;
        }
    ->(target) {
        U value; // the instance value
    }
    compiler.require(source.functions().empty(), "enumerations must not have functions");
    compiler.require(source.bases().empty(), "enumerations must not have base classes");
}


#endif

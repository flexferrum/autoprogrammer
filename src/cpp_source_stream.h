#ifndef CPP_SOURCE_STREAM_H
#define CPP_SOURCE_STREAM_H

#include <iostream>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include <boost/iostreams/filtering_stream.hpp>

namespace codegen 
{
class StreamController;

namespace out
{
using OutParams = std::unordered_map<std::string, std::string>;
}

class CppSourceStream : public boost::iostreams::filtering_ostream
{
public:
    CppSourceStream(std::ostream& os);
    
    void WriteIndent(int indentDelta = 0);
    void Indent(int amout);
    void EnterScope(const std::string& closer, int unindent);
    void ExitScope();
    void WriteHeaderGuard(const std::string& fileName);
    void SetParams(const out::OutParams* params);
    void ResetParams(const out::OutParams *params);
    
private:
    StreamController* m_controller;    
    
    template<typename R>
    friend auto& operator << (CppSourceStream& stream, R (*manip)(CppSourceStream& stream))
    {
        return (*manip)(stream);
    }
    
    template<typename Val>
    friend auto operator << (CppSourceStream& stream, Val&& val) 
            -> std::enable_if_t<!std::is_convertible<decltype(stream << val), CppSourceStream&>::value, CppSourceStream&>
    {
        static_cast<boost::iostreams::filtering_ostream&>(stream) << std::forward<Val>(val);
        return stream;
    }
};

namespace out
{

namespace detail
{
template<typename Fn>
struct Manip
{
    Fn fn;
    friend auto& operator << (CppSourceStream& s, const Manip<Fn>& m)
    {
        return m.fn(s);
    }
};

template<typename Fn>
auto MakeManip(Fn&& fn)
{
    return Manip<Fn>{std::forward<Fn>(fn)};
}

class ExpressionParams
{
public:
    ExpressionParams(const OutParams& p)
        : m_params(&p)
    {       
    }
    
    ~ExpressionParams()
    {
        if (m_stream != nullptr)
            m_stream->ResetParams(m_params);
    }
    
    friend auto& operator << (CppSourceStream& s, const ExpressionParams& m)
    {
        m.m_stream = &s;
        s.SetParams(m.m_params);
        return s;
    }
private:
    const OutParams* m_params;
    mutable CppSourceStream* m_stream = nullptr;
        
};
} // detail

inline CppSourceStream& new_line(CppSourceStream& s)
{
    s << "\n";
    s.WriteIndent();
    return s;
}

inline auto new_line(int extra)
{
    return detail::MakeManip([extra](CppSourceStream& s) -> CppSourceStream& {
        s << "\n";
        s.WriteIndent(extra);
        return s;
    });
}

inline CppSourceStream& indent(CppSourceStream& s)
{
    s.Indent(+1);
    return s;
}

inline CppSourceStream& unindent(CppSourceStream& s)
{
    s.Indent(-1);
    return s;
}

inline auto header_guard(const std::string& headerName)
{
    return detail::MakeManip([headerName](CppSourceStream& s) -> CppSourceStream& {
        s.WriteHeaderGuard(headerName);
        return s;
    });
}

inline auto scope_enter(const std::string& closer, int indent = 1)
{
    return detail::MakeManip([closer, indent](CppSourceStream& s) -> CppSourceStream& {
        s.Indent(indent);
        s.EnterScope(closer, -indent);
        return s;
    });
}

inline CppSourceStream& scope_exit(CppSourceStream& s)
{
    s.ExitScope();
    return s;
}

inline detail::ExpressionParams with_params(const OutParams& params)
{
    return detail::ExpressionParams(params);
}

class ScopedParams
{
public:
    ScopedParams(CppSourceStream& os, OutParams* params)
        : m_stream(&os)
        , m_paramsPtr(params)
    {
        m_stream->SetParams(m_paramsPtr);
    }
    ScopedParams(CppSourceStream& os, OutParams&& params)
        : m_stream(&os)
        , m_params(std::move(params))
        , m_paramsPtr(&m_params)
    {
        m_stream->SetParams(m_paramsPtr);
    }
    ScopedParams(ScopedParams&& params)
        : m_stream(params.m_stream)
        , m_params(std::move(params.m_params))
        , m_paramsPtr(&m_params)
    {
        params.m_stream = nullptr;
        params.m_paramsPtr = nullptr;
    }
    ScopedParams(const ScopedParams&) = delete;
    
    ~ScopedParams()
    {
        if (m_stream != nullptr)
            m_stream->ResetParams(m_paramsPtr);
    }
    
    OutParams& GetParams()
    {
        return *m_paramsPtr;
    }
    
    auto& operator[](const std::string& key)
    {
        return (*m_paramsPtr)[key];
    }

private:
    CppSourceStream* m_stream;
    OutParams m_params;
    OutParams* m_paramsPtr;
};

class StreamScope
{
public:
    StreamScope(std::string prefix, std::string suffix, int indent = 1)
        : m_scopePrefix(std::move(prefix))
        , m_scopeSuffix(std::move(suffix))
        , m_indentLevel(indent)
    {
        if (!m_scopePrefix.empty() && m_scopePrefix[0] == '\n')
        {
            m_prefixOnNewLine = true;
            m_scopePrefix.erase(m_scopePrefix.begin());
        }

        if (!m_scopeSuffix.empty() && m_scopeSuffix[0] == '\n')
        {
            m_suffixOnNewLine = true;
            m_scopeSuffix.erase(m_scopeSuffix.begin());
        }
    }
    
    ~StreamScope()
    {
        if (m_stream != nullptr)
        {
            m_stream->Indent(-m_indentLevel);
            if (m_suffixOnNewLine)
                (*m_stream) << new_line;
            
            (*m_stream) << m_scopeSuffix;
        }
    }
    
private:
    std::string m_scopePrefix;
    std::string m_scopeSuffix;
    int m_indentLevel;
    mutable CppSourceStream* m_stream = nullptr;
    bool m_prefixOnNewLine = true;
    bool m_suffixOnNewLine = true;
    
    friend CppSourceStream& operator << (CppSourceStream& stream, const StreamScope& scope)
    {
        scope.m_stream = &stream;
        if (scope.m_prefixOnNewLine)
            stream << new_line;
        stream << scope.m_scopePrefix;
        stream.Indent(scope.m_indentLevel);
        return stream;
    }
};

class BracedStreamScope : public StreamScope
{
public:
    BracedStreamScope(std::string bracePrefix, std::string braceSuffix, int indent = 1)
        : StreamScope("\n{", "\n}" + braceSuffix, indent)
        , m_bracePrefix(std::move(bracePrefix))
    {
    }
    
    BracedStreamScope(std::string braceSuffix)
        : BracedStreamScope("", std::move(braceSuffix))
    {        
    }
    
private:
    std::string m_bracePrefix;
    
    friend CppSourceStream& operator << (CppSourceStream& stream, const BracedStreamScope& scope)
    {
        stream << scope.m_bracePrefix;
        return stream << static_cast<const StreamScope&>(scope);
    }
};
}
} // codegen
#endif // CPP_SOURCE_STREAM_H

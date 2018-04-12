#include "cpp_source_stream.h"

#include <llvm/Support/Path.h>
#include <llvm/Support/JamCRC.h>
#include <llvm/ADT/ArrayRef.h>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <sstream>
#include <stack>
#include <unordered_set>

namespace codegen 
{
class StreamController : public boost::iostreams::output_filter
{
public:
    enum State
    {
        BypassChars,
        CollectParamName
    };
    
    template<typename Sink>
    bool put(Sink& dest, int c)
    {
        if (m_params.empty())
        {
            m_prevChar = c;
            return boost::iostreams::put(dest, c);
        }

        if (m_currentState == BypassChars)
        {
            if (c == '$')
            {
                m_currentState = CollectParamName;
                m_paramName.clear();
                return true;
            }
            m_prevChar = c;
            return boost::iostreams::put(dest, c);
        }
        else if (m_currentState == CollectParamName)
        {
            if (c == '$')
            {
                bool res = WriteParamValue(dest);
                m_currentState = BypassChars;
                return res;
            }
            m_paramName.append(1, c);
        }
        return true;
    }

    template<typename Sink>
    void close(Sink&)
    {
        ;
    }
    
    void WriteIndent(std::ostream& os, int indentDelta)
    {
        int actualAmount = m_indentLevel + indentDelta;
        if (actualAmount <= 0)
            return;
        
        for (int n = 0; n < actualAmount; ++ n)
            os << "    ";
    }

    void Indent(std::ostream& os, int amount)
    {
        m_indentLevel += amount;
        if (m_indentLevel < 0)
            m_indentLevel = 0;
    }
    
    void EnterScope(const std::string& closer, int unindent)
    {
        m_scopeClosers.push(ScopeCloser{unindent, closer});
    }

    void ExitScope(std::ostream& os)
    {
        if (m_scopeClosers.empty())
            return;
        
        auto closer = m_scopeClosers.top();
        m_scopeClosers.pop();
        Indent(os, closer.indentDelta);
        os << "\n";
        WriteIndent(os, 0);
        os << closer.closeString;
    }

    void WriteHeaderGuard(std::ostream& os, const std::string& filePath)
    {
        std::string fileName = llvm::sys::path::filename(filePath).str();
        boost::algorithm::replace_all(fileName, ".", "_");
        boost::algorithm::replace_all(fileName, "-", "_");
        boost::algorithm::replace_all(fileName, " ", "_");
        boost::algorithm::to_upper(fileName);
        llvm::JamCRC crcCalc;
        crcCalc.update(llvm::ArrayRef<char>(filePath.data(), filePath.size()));
        uint32_t fileHash = crcCalc.getCRC();
        
        std::ostringstream str;
        str << "_" << fileName << "_" << fileHash;
        auto defineName = str.str();
        
        os << "#ifndef " << defineName << "\n";
        os << "#define " << defineName << "\n";
        
        m_scopeClosers.push(ScopeCloser{0, "#endif // " + defineName + "\n"});
    }
    
    void AddParams(const out::OutParams* params)
    {
        m_params.insert(params);
    }
    
    void RemoveParams(const out::OutParams* params)
    {
        m_params.erase(params);
    }
    
private:
    template<typename Sink>
    bool WriteParamValue(Sink&& sink)
    {
        std::string value;
        if (!FindParamValue(m_paramName, value))
            value = m_paramName;
        
        std::streamsize amount = static_cast<std::streamsize>(value.size());
        std::streamsize result = boost::iostreams::write(sink, value.data(), amount);
        return result == amount;
    }
    
    bool FindParamValue(const std::string& valueName, std::string& value)
    {
        for (auto& params : m_params)
        {
            auto p = params->find(valueName);
            if (p != params->end())
            {
                value = p->second;
                return true;
            }
        }
        
        return false;
    }

private:
    int m_indentLevel = 0;
    std::unordered_set<const out::OutParams*> m_params;
    State m_currentState = BypassChars;
    std::string m_paramName;
    char m_prevChar = 0;
    
    struct ScopeCloser
    {
        int indentDelta;
        std::string closeString;
    };
    
    std::stack<ScopeCloser> m_scopeClosers;
};

CppSourceStream::CppSourceStream(std::ostream& os)
{
    this->push(StreamController());
    this->push(os);
    m_controller = this->component<StreamController>(0);
}

void CppSourceStream::WriteIndent(int indentDelta)
{
    m_controller->WriteIndent(*this, indentDelta);
}

void CppSourceStream::Indent(int amout)
{
    m_controller->Indent(*this, amout);
}

void CppSourceStream::EnterScope(const std::string &closer, int unindent)
{
    m_controller->EnterScope(closer, unindent);
}

void CppSourceStream::ExitScope()
{
    m_controller->ExitScope(*this);
}

void CppSourceStream::WriteHeaderGuard(const std::string &fileName)
{
    m_controller->WriteHeaderGuard(*this, fileName);    
}

void CppSourceStream::SetParams(const out::OutParams *params)
{
    m_controller->AddParams(params);
}

void CppSourceStream::ResetParams(const out::OutParams *params)
{
    this->flush();
    m_controller->RemoveParams(params);
}

} // codegen

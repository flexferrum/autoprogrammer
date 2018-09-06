#ifndef CONSOLE_WRITER_H
#define CONSOLE_WRITER_H

#include <iostream>

namespace codegen
{
    
enum MessageType
{
    Notice,
    Warning,
    Error,
    Fatal
};
    
class ConsoleWriter
{
public:
    ConsoleWriter(bool debugMode, std::ostream* consoleStream)
        : m_debugMode(debugMode)
        , m_consoleStream(consoleStream)
        , m_nullStream(nullptr)
    {}
    
    void WriteMessage(MessageType type, const std::string& fileName, unsigned line, unsigned col, const std::string& msg);
    void WriteMessage(MessageType type, const std::string& msg);
    
    
    std::ostream& DebugStream()
    {
        return m_debugMode ? *m_consoleStream : m_nullStream;
    }
    std::ostream& ConsoleStream()
    {
        return *m_consoleStream;
    }
    std::ostream& ConsoleStream(MessageType type);
    std::ostream& ConsoleStream(MessageType type, const std::string& fileName, unsigned line, unsigned col);
    
private:
    bool m_debugMode;
    std::ostream *m_consoleStream;
    std::ostream m_nullStream;
};
}

#endif
#include "console_writer.h"
#include <iostream>

namespace codegen
{
    
void WriteMessageType(std::ostream& os, MessageType type)
{
    switch (type)
    {
    case MessageType::Notice:
        break;
    case MessageType::Warning:
        os << "warning: ";
        break;
    case MessageType::Error:
        os << "error: ";
        break;
    case MessageType::Fatal:
        os << "fatal: ";
        break;
    }
}
    
void ConsoleWriter::WriteMessage(MessageType type, const std::string& fileName, unsigned line, unsigned col, const std::string& msg)
{
    ConsoleStream(type, fileName, line, col) << msg << std::endl;
}

void ConsoleWriter::WriteMessage(MessageType type, const std::string& msg)
{
    ConsoleStream(type) << msg << std::endl;
}

std::ostream& ConsoleWriter::ConsoleStream(MessageType type)
{
    auto& os = ConsoleStream();
    WriteMessageType(os, type);
    return os << ": ";
}

std::ostream& ConsoleWriter::ConsoleStream(MessageType type, const std::string& fileName, unsigned line, unsigned col)
{
    auto& os = ConsoleStream();
    
    os << fileName << ":" << line << ":" << col << ":";
    return ConsoleStream(type);
}
    
}

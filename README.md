# C++ code generation tool
Welcome to **Autoprogrammer**, the **C++ code generation** tool!
This tool helps you dramatically reduce the amount of boilerplate code in your C++ projects. Based on clang frontend, the 'autoprogrammer' parses your C++ source files and generates new set C++ sources. For instance, it generates enum-to-string converting functions for you. Instead of you.

# Table of contents
- [Overview](#overview) 
- [How it works](#how-it-works) 
- [How to use](#how-to-use) 
- [How to build and install](#how-to-build-and-install)

#Overview
Historically, C++ (as a language) has got a lack of compile or runtime introspection, reflection and code injection. In consequence, the easiest task (for example, enum to string conversion) can lead the strong headache. The main purpose of 'autoprogrammer' tool is to eliminate this problem. Completely eliminate. This tool can be integrated in the build toolchain and produce a lot of boring boilerplate source code based on hand-written sources. For instance, can generate enum-string-enum conversion function. Or serialization/deserialization procedures. Or ORM mapping. Or static reflection helpers. And so forth, and so on. Currently, tool supports the following type of generators:
- enum to string (and back) convertors implementation
- pImpl public class methods implementation
- unit test testcase methods (and mock classes) generation _(in development)_

TODO list of generators to support:
- Serialization/deserialization for boost and llvm::yaml
- SQLite ORM
- Helper implementations for popular static reflection libs

# How it works
Autoprogrammer is a clang-based tool. It uses clang frontend for source code analyzes and bunch of Jinja2-like templates for the destination files production. Actually, 'autoprogrammer' compiles your hand-written source code, reflects it to internal structures and then produces new set of C++ source code files via corresponded generator.
In order to process input files correctly, the whole set of compile options and definitions (and include paths as well) should be specified in the autoprogrammer command line.

# How to use

# How to build and install

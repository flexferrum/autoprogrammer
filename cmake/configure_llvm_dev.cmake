if (NOT DEFINED LLVM_ROOT)
    set (LLVM_ROOT $ENV{LLVM_ROOT})
endif ()

if (NOT LLVM_ROOT)
    if (UNIX)
        set (LLVM_ROOT "/usr")
    endif ()
endif ()

# prerequirements for UNIX:
# libclang-5.0-dev
# llvm-5.0-dev
# clang-5.0

message (STATUS "LLVM_ROOT calculated as '${LLVM_ROOT}'")

set (LLVM_INSTALL_ROOT $ENV{LLVM_ROOT} CACHE PATH "Path to LLVM installation root directory")
set (LLVM_SOURCES_ROOT $ENV{LLVM_ROOT} CACHE PATH "Path to LLVM sources root directory")

set (CMAKE_CLANG_DIR ${CLANG_CMAKE_MODULES})

find_path (
    LLVM_CMAKE_MODULES
    AddLLVM.cmake
    PATHS ${LLVM_INSTALL_ROOT}
    PATH_SUFFIXES
        lib/llvm-5.0/cmake/llvm
        lib/llvm-5.0/cmake
        lib64/llvm-5.0/cmake/llvm
        lib64/llvm-5.0/cmake
        lib/llvm/cmake/llvm
        lib/llvm/cmake
        lib64/llvm/cmake/llvm
        lib64/llvm/cmake
        share/llvm-5.0/cmake/llvm
        share/llvm-5.0/cmake
        share/llvm/cmake/llvm
        share/llvm/cmake
)

find_path (
    CLANG_INCLUDE_DIRS
    AST.h
    PATHS ${LLVM_INSTALL_ROOT}
    PATH_SUFFIXES
        lib/llvm-5.0/include/clang/AST
        lib/llvm/include/clang/AST
)

find_library (
    CLANG_CMAKE_LIBRARIES
    clangBasic   
    PATHS ${LLVM_INSTALL_ROOT}
    PATH_SUFFIXES
        lib/llvm-5.0/lib
        lib64/llvm-5.0/lib
        lib/llvm/lib
        lib64/llvm/lib
)

get_filename_component(CLANG_LIBRARY_DIRS "${CLANG_CMAKE_LIBRARIES}" PATH)
get_filename_component(CLANG_INCLUDE_DIRS "${CLANG_INCLUDE_DIRS}" PATH)
get_filename_component(CLANG_INCLUDE_DIRS "${CLANG_INCLUDE_DIRS}" PATH)

message (STATUS "CLANG_CMAKE_LIBRARIES calculated as '${CLANG_CMAKE_LIBRARIES}'")
message (STATUS "CLANG_LIBRARY_DIRS calculated as '${CLANG_LIBRARY_DIRS}'")
message (STATUS "CLANG_INCLUDE_DIRS calculated as '${CLANG_INCLUDE_DIRS}'")
message (STATUS "LLVM_CMAKE_MODULES calculated as '${LLVM_CMAKE_MODULES}'")

link_directories(${CLANG_LIBRARY_DIRS})

list (APPEND CMAKE_MODULE_PATH ${LLVM_CMAKE_MODULES})
list (APPEND CMAKE_MODULE_PATH ${CLANG_CMAKE_MODULES})

set (LLVM_CONFIG_HAS_RTTI YES)
include(AddLLVM)
set (LLVM_CONFIG_HAS_RTTI YES)

# message (FATAL_ERROR "Stop!")

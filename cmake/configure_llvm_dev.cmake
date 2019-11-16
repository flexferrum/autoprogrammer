if (NOT DEFINED LLVM_ROOT)
    set (LLVM_ROOT $ENV{LLVM_ROOT})
endif ()

if (NOT LLVM_ROOT)
    if (UNIX)
        set (LLVM_ROOT "/usr")
    endif ()
endif ()

# prerequirements for UNIX:
# libclang-8-dev
# llvm-8-dev
# clang-8

message (STATUS "LLVM_ROOT calculated as '${LLVM_ROOT}'")

set (LLVM_INSTALL_ROOT $ENV{LLVM_ROOT} CACHE PATH "Path to LLVM installation root directory")
set (LLVM_SOURCES_ROOT $ENV{LLVM_ROOT} CACHE PATH "Path to LLVM sources root directory")

message (STATUS "LLVM_INSTALL_ROOT calculated as '${LLVM_INSTALL_ROOT}'")

set (CMAKE_CLANG_DIR ${CLANG_CMAKE_MODULES})

find_path (
    LLVM_CMAKE_MODULES
    AddLLVM.cmake
    PATHS ${LLVM_INSTALL_ROOT}
    PATH_SUFFIXES
        lib/llvm-8.0/cmake/llvm
        lib/llvm-8.0/cmake
        lib64/llvm-8.0/cmake/llvm
        lib64/llvm-8.0/cmake
        lib/llvm/cmake/llvm
        lib/llvm/cmake
        lib/cmake/llvm
        lib/cmake
        lib64/llvm/cmake/llvm
        lib64/llvm/cmake
        share/llvm-5.0/cmake/llvm
        share/llvm-5.0/cmake
        share/llvm/cmake/llvm
        share/llvm/cmake
)

#if (WINDOWS)
#    set (LLVM_CONFIG_NAME "llvm-config.exe")
#else ()

set (LLVM_CONFIG_NAME "llvm-config" "llvm-config-8.0")

# Taken from CppInsight project by Andreas Fertig
# https://github.com/andreasfertig/cppinsights/blob/master/CMakeLists.txt

find_program(
    LLVM_CONFIG_PATH
    NAMES ${LLVM_CONFIG_NAME}
    PATHS ${LLVM_INSTALL_ROOT}
    PATH_SUFFIXES
        bin
    )

message (STATUS "LLVM_CONFIG_PATH calculated as '${LLVM_CONFIG_PATH}'")
if(NOT LLVM_CONFIG_PATH)
    message(FATAL_ERROR "llvm-config not found -- ${LLVM_CONFIG_PATH}: ${LLVM_CONFIG_NAME}")
else()
    message(STATUS "Found LLVM_CONFIG_PATH: ${LLVM_CONFIG_PATH}")
endif()

function(llvm_config VARNAME switch)
    set(CONFIG_COMMAND "${LLVM_CONFIG_PATH}" "${switch}")

    execute_process(
        COMMAND ${CONFIG_COMMAND} ${LIB_TYPE}
        RESULT_VARIABLE HAD_ERROR
        OUTPUT_VARIABLE CONFIG_OUTPUT
    )

    if (HAD_ERROR)
        string(REPLACE ";" " " CONFIG_COMMAND_STR "${CONFIG_COMMAND}")
        message(STATUS "${CONFIG_COMMAND_STR}")
        message(FATAL_ERROR "llvm-config failed with status ${HAD_ERROR}")
    endif()

    # replace linebreaks with semicolon
    string(REGEX REPLACE
        "[ \t]*[\r\n]+[ \t]*" ";"
        CONFIG_OUTPUT ${CONFIG_OUTPUT})

    # make all includes system include to prevent the compiler to warn about issues in LLVM/Clang
    string(REGEX REPLACE "-I" "-isystem" CONFIG_OUTPUT "${CONFIG_OUTPUT}")

    # remove certain options clang doesn't like
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        string(REGEX REPLACE "-Wl,--no-keep-files-mapped" "" CONFIG_OUTPUT "${CONFIG_OUTPUT}")
        string(REGEX REPLACE "-Wl,--no-map-whole-files" "" CONFIG_OUTPUT "${CONFIG_OUTPUT}")
        string(REGEX REPLACE "-fuse-ld=gold" "" CONFIG_OUTPUT "${CONFIG_OUTPUT}")
    endif()

    # make result available outside
    set(${VARNAME} ${CONFIG_OUTPUT} PARENT_SCOPE)

    # Optionally output the configured value
    message(STATUS "llvm_config(${VARNAME})=>${CONFIG_OUTPUT}")

    # cleanup
    unset(CONFIG_COMMAND)
endfunction(llvm_config)

set (CLANG_INCLUDE_DIRS)

find_path (
    CLANG_INCLUDE_DIRS
    ASTConsumer.h
    PATHS ${LLVM_INSTALL_ROOT}
    PATH_SUFFIXES
        lib/llvm-5.0/include/clang/AST
        lib/llvm/include/clang/AST
        lib/include/clang/AST
        include/clang/AST
    NO_DEFAULT_PATH
    NO_CMAKE_ENVIRONMENT_PATH
    NO_CMAKE_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
    NO_CMAKE_SYSTEM_PATH
    NO_CMAKE_FIND_ROOT_PATH
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
        lib
        lib64
)

get_filename_component(CLANG_LIBRARY_DIRS "${CLANG_CMAKE_LIBRARIES}" PATH)
message (STATUS "CLANG_INCLUDE_DIRS calculated as '${CLANG_INCLUDE_DIRS}'")
get_filename_component(CLANG_INCLUDE_DIRS "${CLANG_INCLUDE_DIRS}" PATH)
message (STATUS "CLANG_INCLUDE_DIRS calculated as '${CLANG_INCLUDE_DIRS}'")
get_filename_component(CLANG_INCLUDE_DIRS "${CLANG_INCLUDE_DIRS}" PATH)

message (STATUS "CLANG_CMAKE_LIBRARIES calculated as '${CLANG_CMAKE_LIBRARIES}'")
message (STATUS "CLANG_LIBRARY_DIRS calculated as '${CLANG_LIBRARY_DIRS}'")
message (STATUS "CLANG_INCLUDE_DIRS calculated as '${CLANG_INCLUDE_DIRS}'")
message (STATUS "LLVM_CMAKE_MODULES calculated as '${LLVM_CMAKE_MODULES}'")

link_directories(${CLANG_LIBRARY_DIRS} ${LLVM_LDFLAGS})

set (LLVM_CONFIG_HAS_RTTI YES)

llvm_config(LLVM_LIBS "--libs")
llvm_config(LLVM_SYSTEM_LIBS "--system-libs")

if (WIN32 OR WIN64)
    message (STATUS " >>>>>>>> 1111111")
    list (APPEND CMAKE_MODULE_PATH ${LLVM_CMAKE_MODULES})
    include(AddLLVM)
#    include (clang-windows/ClangConfig)
    set (EXTRA_LIBS LLVMLTO 
LLVMPasses
LLVMObjCARCOpts
LLVMSymbolize
LLVMDebugInfoPDB
LLVMDebugInfoDWARF
LLVMFuzzMutate
LLVMMCA
LLVMTableGen
LLVMDlltoolDriver
LLVMLineEditor
LLVMXRay
LLVMOrcJIT
LLVMCoverage
LLVMMIRParser
LLVMObjectYAML
LLVMLibDriver
LLVMOption
LLVMOptRemarks
LLVMWindowsManifest
LLVMTextAPI
LLVMX86Disassembler
LLVMX86AsmParser
LLVMX86CodeGen
LLVMGlobalISel
LLVMSelectionDAG
LLVMAsmPrinter
LLVMX86Desc
LLVMMCDisassembler
LLVMX86Info
LLVMX86AsmPrinter
LLVMX86Utils
LLVMMCJIT
LLVMInterpreter
LLVMExecutionEngine
LLVMRuntimeDyld
LLVMCodeGen
LLVMTarget
LLVMCoroutines
LLVMipo
LLVMInstrumentation
LLVMVectorize
LLVMScalarOpts
LLVMLinker
LLVMIRReader
LLVMAsmParser
LLVMInstCombine
LLVMBitWriter
LLVMAggressiveInstCombine
LLVMTransformUtils
LLVMAnalysis
LLVMProfileData
LLVMObject
LLVMMCParser
LLVMMC
LLVMDebugInfoCodeView
LLVMDebugInfoMSF
LLVMBitReader
LLVMCore
LLVMBinaryFormat
LLVMSupport
LLVMDemangle
psapi shell32 ole32 uuid advapi32 version
)
#    set (EXTRA_LIBS "${EXTRA_LIBS} ${LLVM_LIBS} ${LLVM_SYSTEM_LIBS
    message(STATUS ">>>>>>..... ${EXTRA_LIBS}")
else ()
    message (STATUS " >>>>>>>> 2222222")
    llvm_config(LLVM_CXXFLAGS "--cxxflags")
    llvm_config(LLVM_LDFLAGS "--ldflags")
    llvm_config(LLVM_LIBS "--libs")
    llvm_config(LLVM_LIBDIR "--libdir")
    llvm_config(LLVM_INCLUDE_DIR "--includedir")
    llvm_config(LLVM_SYSTEM_LIBS "--system-libs")
    llvm_config(LLVM_PACKAGE_VERSION "--version")

    string (REPLACE "-std=c++0x" "-std=c++14" LLVM_CXXFLAGS "${LLVM_CXXFLAGS}")
    string (REPLACE "-std=c++11" "-std=c++14" LLVM_CXXFLAGS "${LLVM_CXXFLAGS}")
    string (REPLACE "-fno-rtti" "" LLVM_CXXFLAGS "${LLVM_CXXFLAGS}")
    string (REPLACE "-fno-exceptions" "" LLVM_CXXFLAGS "${LLVM_CXXFLAGS}")


    macro(add_llvm_executable name)
        add_executable( ${name} ${ARGN} )
    endmacro(add_clang_executable)
    set (EXTRA_LIBS ${LLVM_LDFLAGS} ${EXTRA_LIBS} ${LLVM_LIBS} ${LLVM_SYSTEM_LIBS})
endif ()
set (LLVM_CONFIG_HAS_RTTI YES)

# message (FATAL_ERROR "Stop!")

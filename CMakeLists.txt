project (FlexLib-CodeGen VERSION 0.1 LANGUAGES CXX)

set(CODEGEN_BIN_NAME "fl-codegen")
set(LLVM_INSTALL_ROOT $ENV{LLVM_ROOT} CACHE PATH "Path to LLVM installation root directory")
set(LLVM_SOURCES_ROOT $ENV{LLVM_ROOT} CACHE PATH "Path to LLVM sources root directory")

list (APPEND CMAKE_MODULE_PATH ${LLVM_INSTALL_ROOT}/lib/cmake/llvm)
list (APPEND CMAKE_MODULE_PATH ${LLVM_INSTALL_ROOT}/lib/cmake/clang)

include(collect_sources)
include(AddLLVM)
include(ClangConfig)

CollectSources(Sources Headers ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src)

set (Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED COMPONENTS iostreams)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
    message(STATUS "CodeGenerator is compiled with enabled exceptions")
endif()

include_directories(
    ${CLANG_INCLUDE_DIRS}
    ${LLVM_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    )


add_llvm_executable(${CODEGEN_BIN_NAME}
    ${Sources}
    ${Headers}
    )

target_compile_options(${CODEGEN_BIN_NAME}
  PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/EHsc>
)

target_link_libraries(${CODEGEN_BIN_NAME}
    clangAST
    clangBasic
    clangDriver
    clangFrontend
    clangRewriteFrontend
    clangStaticAnalyzerFrontend
    clangTooling
    ${Boost_LIBRARIES}
    )

add_subdirectory(test)

install(TARGETS ${CODEGEN_BIN_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib/static)

cmake_minimum_required(VERSION 3.0)

project (Autoprogrammer VERSION 0.1 LANGUAGES CXX)

list (APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(collect_sources)

set(GTEST_ROOT $ENV{GTEST_DIR} CACHE PATH "Path to GTest/GMock library root")
set(BOOST_ROOT $ENV{BOOST_DIR} CACHE PATH "Path to boost library root")
set(CXX_STANDARD "14" CACHE STRING "C++ standard conformance for library build")

set(CODEGEN_BIN_NAME "fl-codegen")

include (configure_llvm_dev)

include(collect_sources)

if(NOT "${GTEST_ROOT}" STREQUAL "")
    list (APPEND CMAKE_PREFIX_PATH ${GTEST_ROOT})
    set (Gtest_DIR ${GTEST_ROOT})
    message(STATUS "GTest library search path: ${Gtest_DIR}")
endif()


if(NOT "${BOOST_ROOT}" STREQUAL "")
    list (APPEND CMAKE_PREFIX_PATH ${BOOST_ROOT})
    set (Boost_DIR ${BOOST_ROOT})
    message(STATUS "BOOST library search path: ${Boost_DIR}")
endif()

if(NOT "${FLEXLIB_ROOT}" STREQUAL "")
    list (APPEND CMAKE_PREFIX_PATH ${FLEXLIB_ROOT})
    set (FlexLib_DIR ${FLEXLIB_ROOT})
else()
    set (FlexLib_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/flex_lib)
endif()
message(STATUS "FlexLib library search path: ${FlexLib_DIR}")
set (FLEXLIB_INCLUDE_DIR ${FlexLib_DIR}/include)

find_package(GTest)
find_package(Boost)

message (STATUS "##### COMPILER = ${CMAKE_CXX_COMPILER_ID}")
message (STATUS "##### COMPILER = ${CMAKE_CXX_COMPILER_VERSION}")

if (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang" OR ${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
    if (${CXX_STANDARD} STREQUAL "17" OR ${CXX_STANDARD} STREQUAL "1z")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
        message(STATUS "FlexLib is building with C++17 enabled")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
        message(STATUS "FlexLib is building with C++14 enabled")
    endif()
endif()

CollectSources(Sources Headers ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src)

set (Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED COMPONENTS iostreams)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
    message(STATUS "CodeGenerator is compiled with enabled exceptions")
endif()

add_subdirectory(thirdparty)

include_directories(
    ${CLANG_INCLUDE_DIRS}
    ${LLVM_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${GTEST_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
    ${FLEXLIB_INCLUDE_DIR}
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
    clangFormat
    ${Boost_LIBRARIES}
#    Jinja2CppLight
    )

get_target_property(CODEGEN_COMPILE_OPTIONS ${CODEGEN_BIN_NAME} COMPILE_FLAGS)
string (REPLACE "-fno-rtti" "" CODEGEN_COMPILE_OPTIONS "${CODEGEN_COMPILE_OPTIONS}")
string (REPLACE "-fno-exceptions" "" CODEGEN_COMPILE_OPTIONS "${CODEGEN_COMPILE_OPTIONS}")
set_target_properties(${CODEGEN_BIN_NAME} PROPERTIES COMPILE_FLAGS "${CODEGEN_COMPILE_OPTIONS}")
message (STATUS ">>>>> CODEGEN_COMPILE_OPTIONS : ${CODEGEN_COMPILE_OPTIONS}")
message (STATUS ">>>>> CMAKE_CXX_FLAGS : ${CMAKE_CXX_FLAGS}")
message (STATUS ">>>>> CMAKE_CXX_FLAGS_RELEASE : ${CMAKE_CXX_FLAGS_RELEASE}")
message (STATUS ">>>>> CMAKE_CXX_FLAGS_RELWITHDEBINFO : ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")

add_subdirectory(test)

install(TARGETS ${CODEGEN_BIN_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib/static)

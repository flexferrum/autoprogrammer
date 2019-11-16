cmake_minimum_required(VERSION 3.0)

project (Autoprogrammer VERSION 0.1 LANGUAGES CXX)

list (APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_CURRENT_BINARY_DIR})
include(collect_sources)
include(conan)

message(STATUS ">>>>>> ${MSVC_VERSION}")

if (NOT MSVC)
    set (CONAN_SETTINGS SETTINGS compiler.libcxx=libstdc++11)
endif ()

conan_cmake_run(REQUIRES 
				"variant-lite/[>=1.2.2]"
				"expected-lite/[>=0.3.0]"
				"optional-lite/[>=3.2.0]"
				"string-view-lite/[>=1.3.0]"
				"boost/1.71.0"
				"rapidjson/1.1.0"
				"fmt/[>=5.3]" 
				"gtest/[>=1.8]" 
			BASIC_SETUP
                ${CONAN_SETTINGS}
				GENERATORS 
					cmake_find_package
					cmake_paths
                OPTIONS 
                    jinja2cpp:shared=False
                    gtest:shared=False
                BUILD missing)


set(CXX_STANDARD "14" CACHE STRING "C++ standard conformance for library build")

set(CODEGEN_BIN_NAME "fl-codegen")

include (configure_llvm_dev)

include(collect_sources)

set (FlexLib_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/flex_lib)
message(STATUS "FlexLib library search path: ${FlexLib_DIR}")
set (FLEXLIB_INCLUDE_DIR ${FlexLib_DIR}/include)

set (CODEGEN_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

find_package(GTest)

message (STATUS "##### COMPILER = ${CMAKE_CXX_COMPILER_ID}")
message (STATUS "##### COMPILER = ${CMAKE_CXX_COMPILER_VERSION}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LLVM_CXXFLAGS}")

CollectSources(Sources Headers ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src)
CollectSources(PublicSources PublicHeaders ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)

set (Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED COMPONENTS iostreams filesystem system)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /bigobj")
    message(STATUS "CodeGenerator is compiled with enabled exceptions")
else()
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-backtrace-limit=0")
endif()

find_package(Threads)

add_subdirectory(thirdparty)

include_directories(
    SYSTEM
    ${CLANG_INCLUDE_DIRS}
    ${LLVM_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/expected-lite/include
    )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${FLEXLIB_INCLUDE_DIR}
    )


add_llvm_executable(${CODEGEN_BIN_NAME}
    ${Sources}
    ${Headers}
    ${PublicHeaders}
    )

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
    target_compile_options(${CODEGEN_BIN_NAME} PUBLIC /EHsc)
else()
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-backtrace-limit=0")
endif()

# target_compile_options(${CODEGEN_BIN_NAME}
#  PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/EHsc>
# )

target_link_libraries(${CODEGEN_BIN_NAME}
    clangFormat
    clangASTMatchers
    clangTooling
    clangToolingCore
    clangRewriteFrontend
    clangStaticAnalyzerFrontend
    clangFrontend
    clangTooling
    clangFormat
    clangDriver
    clangSerialization
    clangParse
    clangSema
    clangAnalysis
    clangEdit
    clangAST
    clangLex
    clangBasic
    clangRewrite
	clangToolingInclusions
    ${EXTRA_LIBS}
    libboost_iostreams
	libboost_filesystem
	libboost_system
    Threads::Threads
    jinja2cpp
    )

get_target_property(CODEGEN_COMPILE_OPTIONS ${CODEGEN_BIN_NAME} COMPILE_FLAGS)
message (STATUS ">>>>> CODEGEN_COMPILE_OPTIONS (1): ${CODEGEN_COMPILE_OPTIONS}")
string (REPLACE "-fno-rtti" "" CODEGEN_COMPILE_OPTIONS "${CODEGEN_COMPILE_OPTIONS}")
string (REPLACE "-fno-exceptions" "" CODEGEN_COMPILE_OPTIONS "${CODEGEN_COMPILE_OPTIONS}")
string (REPLACE "/EHs-c-" "" CODEGEN_COMPILE_OPTIONS "${CODEGEN_COMPILE_OPTIONS}")
string (REPLACE "-EHs-c-" "" CODEGEN_COMPILE_OPTIONS "${CODEGEN_COMPILE_OPTIONS}")
string (REPLACE "/GR-" "" CODEGEN_COMPILE_OPTIONS "${CODEGEN_COMPILE_OPTIONS}")
string (REPLACE "-GR-" "" CODEGEN_COMPILE_OPTIONS "${CODEGEN_COMPILE_OPTIONS}")
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
	set(CODEGEN_COMPILE_OPTIONS "${CODEGEN_COMPILE_OPTIONS} /W0")
endif()
set_target_properties(${CODEGEN_BIN_NAME} PROPERTIES COMPILE_FLAGS "${CODEGEN_COMPILE_OPTIONS}")
message (STATUS ">>>>> CODEGEN_COMPILE_OPTIONS (2): ${CODEGEN_COMPILE_OPTIONS}")
message (STATUS ">>>>> CMAKE_CXX_FLAGS : ${CMAKE_CXX_FLAGS}")
message (STATUS ">>>>> CMAKE_CXX_FLAGS_RELEASE : ${CMAKE_CXX_FLAGS_RELEASE}")
message (STATUS ">>>>> CMAKE_CXX_FLAGS_RELWITHDEBINFO : ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")

enable_testing()

add_subdirectory(test)

install(TARGETS ${CODEGEN_BIN_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib/static)

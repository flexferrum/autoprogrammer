project (thirdparty_libs)

project(Jinja2Cpp)

include(collect_sources)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/Jinja2Cpp/include
    ${Boost_INCLUDE_DIRS}
    ${GTEST_INCLUDE_DIRS}
    )

CollectSources(Sources Headers ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/Jinja2Cpp/src)
CollectSources(PublicSources PublicHeaders ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/Jinja2Cpp/include)
CollectSources(TestSources TestHeaders ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/Jinja2Cpp/test)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GR-")
    message(STATUS "CodeGenerator is compiled with enabled exceptions")
endif()

add_library(Jinja2Cpp STATIC
    ${Sources}
    ${Headers}
    ${PublicHeaders}
)

target_include_directories(Jinja2Cpp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/Jinja2Cpp/include)

# add_executable(jinja2cpp_unittests ${TestSources} ${TestHeaders})
# target_link_libraries(jinja2cpp_unittests ${GTEST_BOTH_LIBRARIES} Boost::system Boost::filesystem Threads::Threads)
# target_link_libraries(jinja2cpp_unittests Jinja2Cpp)


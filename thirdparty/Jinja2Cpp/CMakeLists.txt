project(Jinja2CppLight)

execute_process(COMMAND python -V ERROR_VARIABLE python_test)
if(${python_test} MATCHES "Python 2\\.7\\..*")
    #message("Python 2.7 found")
    OPTION(PYTHON_AVAILABLE "Facilitates development, not needed for normal build." ON)
else()
    OPTION(PYTHON_AVAILABLE "Facilitates development, not needed for normal build." OFF)
endif()

include_directories(
    .
    src
    ${Boost_INCLUDE_DIRS}
    )

add_library(Jinja2CppLight STATIC src/Jinja2CppLight.cpp src/stringhelper.cpp)

if(PYTHON_AVAILABLE)
    add_custom_target(
        cog
        python ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/cogapp/cog.py -q -I ${CMAKE_CURRENT_SOURCE_DIR}/cog-batteries -r ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/test/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/test/*.h
    )
    add_dependencies(Jinja2CppLight cog)
endif(PYTHON_AVAILABLE)

if(UNIX)
    add_library(jinja2cpplight_gtest STATIC thirdparty/gtest/gtest-all.cc)
    target_link_libraries(jinja2cpplight_gtest pthread)
else()
    add_library(jinja2cpplight_gtest thirdparty/gtest/gtest-all.cc)
endif()
target_include_directories(jinja2cpplight_gtest PRIVATE thirdparty/gtest)
target_include_directories(Jinja2CppLight INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(jinja2cpplight_unittests thirdparty/gtest/gtest_main.cc test/testJinja2CppLight.cpp test/teststringhelper.cpp)
target_link_libraries(jinja2cpplight_unittests jinja2cpplight_gtest)
target_link_libraries(jinja2cpplight_unittests Jinja2CppLight)
target_include_directories(jinja2cpplight_unittests PRIVATE thirdparty/gtest)

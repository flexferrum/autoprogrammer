project (metaclasses_codegen_tests)

include (collect_sources)

CollectSources(Sources Headers ${CMAKE_CURRENT_SOURCE_DIR}  ${CMAKE_CURRENT_SOURCE_DIR})

set (CODEGEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/codegen)

file (MAKE_DIRECTORY ${CODEGEN_DIR}/generated)

include_directories(
    ${CODEGEN_DIR}
    ${CODEGEN_ROOT_DIR}/include
    )

set (METACLASSES_GEN_FILE ${CODEGEN_DIR}/generated/test_interface.meta.h)

add_custom_command(OUTPUT ${METACLASSES_GEN_FILE}
    COMMAND $<TARGET_FILE:${CODEGEN_BIN_NAME}> -gen-metaclasses -show-clang-diag -ohdr ${METACLASSES_GEN_FILE} -input ${CMAKE_CURRENT_SOURCE_DIR}/test_interface.h -- clang-cl -std=c++14 -x c++ ${CMAKE_CXX_FLAGS} -I ${CODEGEN_ROOT_DIR}/include -I ${FLEXLIB_INCLUDE_DIR}
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/test_interface.h
    DEPENDS ${CODEGEN_BIN_NAME}
    COMMENT "Generating metaclasses implementation for ${CMAKE_CURRENT_SOURCE_DIR}/test_interface.h"
    )

if (UNIX)
    include(CMakeFindDependencyMacro)
    find_package(Threads)
    set (EXTRA_TEST_LIBS "Threads::Threads")
endif ()

add_executable(${PROJECT_NAME}
    ${Sources}
    ${Headers}
    ${METACLASSES_GEN_FILE}
    )

target_link_libraries(${PROJECT_NAME} gtest::gtest ${EXTRA_TEST_LIBS})

add_test(${PROJECT_NAME} ${PROJECT_NAME})

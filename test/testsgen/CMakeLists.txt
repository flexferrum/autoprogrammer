project (testsgen_codegen_tests)

include (collect_sources)

CollectSources(Sources Headers ${CMAKE_CURRENT_SOURCE_DIR}  ${CMAKE_CURRENT_SOURCE_DIR})

set (CODEGEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/codegen)

file (MAKE_DIRECTORY ${CODEGEN_DIR}/generated)

include_directories(
    ${CODEGEN_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    )

set (TESTS_GEN_HDRFILE01 ${CODEGEN_DIR}/generated/testgen_01.h)
set (TESTS_GEN_SRCFILE01 ${CODEGEN_DIR}/generated/testgen_01.cpp)

add_custom_command(OUTPUT ${TESTS_GEN_HDRFILE01} ${TESTS_GEN_SRCFILE01}
    COMMAND $<TARGET_FILE:${CODEGEN_BIN_NAME}> -gen-tests -osrc ${TESTS_GEN_SRCFILE01} -ohdr ${TESTS_GEN_HDRFILE01} -input ${CMAKE_CURRENT_SOURCE_DIR}/testclasses_01.h -- clang-cl -std=c++14 -x c++ ${CMAKE_CXX_FLAGS}
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/testclasses_01.h
    DEPENDS ${CODEGEN_BIN_NAME}
    COMMENT "Generating test cases for ${CMAKE_CURRENT_SOURCE_DIR}/testclasses_01.h"
    )

add_executable(${PROJECT_NAME}
    ${Sources}
    ${Headers}
    ${TESTS_GEN_HDRFILE01}
    ${TESTS_GEN_SRCFILE01}
    )

target_link_libraries(${PROJECT_NAME} ${GTEST_BOTH_LIBRARIES})

add_test(${PROJECT_NAME} ${PROJECT_NAME})

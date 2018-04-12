project (enum2string_codegen_tests)

include (collect_sources)

CollectSources(Sources Headers ${CMAKE_CURRENT_SOURCE_DIR}  ${CMAKE_CURRENT_SOURCE_DIR})

set (CODEGEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/codegen)

file (MAKE_DIRECTORY ${CODEGEN_DIR}/generated)

include_directories(
    ${CODEGEN_DIR}
    )

set (ENUM_CONV_FILE ${CODEGEN_DIR}/generated/enum_conv_gen.h)

add_custom_command(OUTPUT ${ENUM_CONV_FILE}
    COMMAND $<TARGET_FILE:${CODEGEN_BIN_NAME}> -gen-enum2string -ohdr ${ENUM_CONV_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/test_enums.h -- clang-cl -std=c++14 -x c++ ${CMAKE_CXX_FLAGS}
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/test_enums.h
    DEPENDS ${CODEGEN_BIN_NAME}
    COMMENT "Generating enum2string converters for ${CMAKE_CURRENT_SOURCE_DIR}/test_enums.h"
    )

add_executable(${PROJECT_NAME}
    ${Sources}
    ${Headers}
    ${ENUM_CONV_FILE}
    )

target_link_libraries(${PROJECT_NAME} ${GTEST_BOTH_LIBRARIES})

add_test(${PROJECT_NAME} ${PROJECT_NAME})

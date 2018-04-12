project (pimpl_codegen_tests)

include (collect_sources)

CollectSources(Sources Headers ${CMAKE_CURRENT_SOURCE_DIR}  ${CMAKE_CURRENT_SOURCE_DIR})

set (CODEGEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/codegen)

file (MAKE_DIRECTORY ${CODEGEN_DIR}/generated)

include_directories(
    ${CODEGEN_DIR}
    )

set (PIMPL_GEN_FILE ${CODEGEN_DIR}/generated/pimpl_gen.cpp)

add_custom_command(OUTPUT ${PIMPL_GEN_FILE}
    COMMAND $<TARGET_FILE:${CODEGEN_BIN_NAME}> -gen-pimpl -ohdr ${PIMPL_GEN_FILE} -eh ${CMAKE_CURRENT_SOURCE_DIR}/test_pimpl_impl.h ${CMAKE_CURRENT_SOURCE_DIR}/test_pimpl.h -- clang-cl -std=c++14 -x c++ ${CMAKE_CXX_FLAGS} -I ${FLEX_LIB_INCLUDE_ROOT}
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/test_pimpl.h
    DEPENDS ${CODEGEN_BIN_NAME}
    COMMENT "Generating pimpl default implementation for ${CMAKE_CURRENT_SOURCE_DIR}/test_pimpl.h"
    )

add_executable(${PROJECT_NAME}
    ${Sources}
    ${Headers}
    ${PIMPL_GEN_FILE}
    )

target_link_libraries(${PROJECT_NAME} GTest::GTest GTest::Main flex_lib)

add_test(${PROJECT_NAME} ${PROJECT_NAME})

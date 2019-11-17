project (serialization_tests)

include (collect_sources)

CollectSources(Sources Headers ${CMAKE_CURRENT_SOURCE_DIR}  ${CMAKE_CURRENT_SOURCE_DIR})

set (CODEGEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/codegen)

file (MAKE_DIRECTORY ${CODEGEN_DIR}/generated)

include_directories(
    ${CODEGEN_DIR}
    )

find_package(rapidjson)

macro(GenerateFile OUTPUT_FILE_NAME TEMPLATE_NAME)
add_custom_command(OUTPUT ${CODEGEN_DIR}/generated/${OUTPUT_FILE_NAME}
    COMMAND $<TARGET_FILE:${CODEGEN_BIN_NAME}> -gen-serialization -ohdr ${CODEGEN_DIR}/generated/${OUTPUT_FILE_NAME} -input ${CMAKE_CURRENT_SOURCE_DIR}/test_structs.h -tpl-dir ${CMAKE_CURRENT_SOURCE_DIR}/templates -tpl ${TEMPLATE_NAME} -- clang-cl -std=c++14 -x c++ ${CMAKE_CXX_FLAGS}
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/test_structs.h
    DEPENDS ${CODEGEN_BIN_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/templates/${TEMPLATE_NAME}
    COMMENT "Generating serialization converters for ${CMAKE_CURRENT_SOURCE_DIR}/test_structs.h with template ${TEMPLATE_NAME}"
    )
    
    list(APPEND GENERATED_FILES ${CODEGEN_DIR}/generated/${OUTPUT_FILE_NAME})
endmacro()

GenerateFile(json_serialization.h json_serialization.h.j2tpl)

if (UNIX)
    include(CMakeFindDependencyMacro)
    find_dependency(Threads)
    set (EXTRA_TEST_LIBS "Threads::Threads")
endif ()

add_executable(${PROJECT_NAME}
    ${Sources}
    ${Headers}
    ${GENERATED_FILES}
    )

target_link_libraries(${PROJECT_NAME} gtest::gtest rapidjson::rapidjson ${EXTRA_TEST_LIBS})

add_test(${PROJECT_NAME} ${PROJECT_NAME})

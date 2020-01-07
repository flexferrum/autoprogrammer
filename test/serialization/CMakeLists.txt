project (serialization_tests)

include (collect_sources)

CollectSources(Sources Headers ${CMAKE_CURRENT_SOURCE_DIR}  ${CMAKE_CURRENT_SOURCE_DIR})

set (CODEGEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/codegen)
set (METACLASSES_GEN_FILE ${CODEGEN_DIR}/generated/serialization.meta.h)

file (MAKE_DIRECTORY ${CODEGEN_DIR}/generated)

include_directories(
    ${CODEGEN_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CODEGEN_ROOT_DIR}/include
    )

find_package(rapidjson)
find_package(protobuf)
find_package(Boost)

macro(GenerateFile OUTPUT_FILE_NAME TEMPLATE_NAME)
add_custom_command(OUTPUT ${CODEGEN_DIR}/generated/${OUTPUT_FILE_NAME}
    COMMAND $<TARGET_FILE:${CODEGEN_BIN_NAME}> -gen-serialization -ohdr ${CODEGEN_DIR}/generated/${OUTPUT_FILE_NAME} -input ${CMAKE_CURRENT_SOURCE_DIR}/test_structs.h -tpl-dir ${CMAKE_CURRENT_SOURCE_DIR}/templates -tpl ${TEMPLATE_NAME} ${ARGV2} ${ARGV3} -- clang-cl -std=c++14 -x c++ ${CMAKE_CXX_FLAGS}
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/test_structs.h
    DEPENDS ${CODEGEN_BIN_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/templates/${TEMPLATE_NAME}
    COMMENT "Generating serialization converters for ${CMAKE_CURRENT_SOURCE_DIR}/test_structs.h with template ${TEMPLATE_NAME}"
    )
    
    list(APPEND GENERATED_FILES ${CODEGEN_DIR}/generated/${OUTPUT_FILE_NAME})
endmacro()

GenerateFile(json_serialization.h json_serialization.h.j2tpl)
GenerateFile(protobuf_serialization.proto protobuf_serialization.proto.j2tpl "-tpl-param" "package_name=serialization_test")
GenerateFile(protobuf_serialization.cpp protobuf_serialization.cpp.j2tpl "-tpl-param" "package_name=serialization_test")
GenerateFile(protobuf_serialization.h protobuf_serialization.h.j2tpl)

add_custom_command(OUTPUT ${METACLASSES_GEN_FILE}
    COMMAND $<TARGET_FILE:${CODEGEN_BIN_NAME}> -gen-metaclasses -show-clang-diag -ohdr ${METACLASSES_GEN_FILE} -input ${CMAKE_CURRENT_SOURCE_DIR}/meta_test_structs.h -- clang-cl -std=c++14 -x c++ ${CMAKE_CXX_FLAGS} -I ${CODEGEN_ROOT_DIR}/include -I ${FLEXLIB_INCLUDE_DIR} -I ${rapidjson_INCLUDE_DIRS}
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/meta_test_structs.h
    DEPENDS ${CODEGEN_BIN_NAME}
    COMMENT "Generating metaclasses implementation for ${CMAKE_CURRENT_SOURCE_DIR}/meta_test_structs.h"
    )


# list(APPEND GENERATED_FILES ${CODEGEN_DIR}/generated/protobuf_serialization.cpp)
set (PROTOBUF_IMPORT_DIRS ${CODEGEN_DIR}/generated)

protobuf_generate_cpp(
    GENERATED_PROTO_SOURCES
    GENERATED_PROTO_HEADERS
    ${CODEGEN_DIR}/generated/protobuf_serialization.proto
#    PROTOC_OUT_DIR  ${CODEGEN_DIR}/generated/protobuf
#    IMPORT_DIRS ${CODEGEN_DIR}/generated
)

list(APPEND GENERATED_FILES ${GENERATED_PROTO_SOURCES}  ${GENERATED_PROTO_HEADERS} ${METACLASSES_GEN_FILE})

if (UNIX)
    include(CMakeFindDependencyMacro)
    find_dependency(Threads)
    set (EXTRA_TEST_LIBS "Threads::Threads")
endif ()

add_executable(${PROJECT_NAME}          
    ${Sources}
    ${Headers}
    ${GENERATED_FILES}
    ${GENERATED_PROTO_FILES}
    )

target_link_libraries(${PROJECT_NAME} GTest::GTest rapidjson::rapidjson protobuf::libprotobuf Boost::Boost ${EXTRA_TEST_LIBS})

add_test(${PROJECT_NAME} ${PROJECT_NAME})

project (serialization_tests)

include (collect_sources)

CollectSources(Sources Headers ${CMAKE_CURRENT_SOURCE_DIR}  ${CMAKE_CURRENT_SOURCE_DIR})

set (CODEGEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/codegen)

file (MAKE_DIRECTORY ${CODEGEN_DIR}/generated)

include_directories(
    ${CODEGEN_DIR}
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

# list(APPEND GENERATED_FILES ${CODEGEN_DIR}/generated/protobuf_serialization.cpp)

protobuf_generate(
    LANGUAGE cpp
    PROTOS ${CODEGEN_DIR}/generated/protobuf_serialization.proto
    PROTOC_OUT_DIR  ${CODEGEN_DIR}/generated
    IMPORT_DIRS ${CODEGEN_DIR}/generated
    OUT_VAR GENERATED_PROTO_FILES
)

message (STATUS ">>>>>>#######1111 ${GENERATED_FILES}")
# list(APPEND GENERATED_FILES ${CODEGEN_DIR}/generated/protobuf/protobuf_serialization.pb.cc ${CODEGEN_DIR}/generated/protobuf/protobuf_serialization.pb.h)
message (STATUS ">>>>>>#######2222 ${GENERATED_FILES}")

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

target_link_libraries(${PROJECT_NAME} gtest::gtest rapidjson::rapidjson protobuf::libprotobuf Boost::Boost ${EXTRA_TEST_LIBS})

add_test(${PROJECT_NAME} ${PROJECT_NAME})

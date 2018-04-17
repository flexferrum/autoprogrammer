project (thirdparty_libs)

add_subdirectory(Jinja2CppLight)
## Build Jinja2CppLight
#if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/Jinja2CppLight)
#        make_directory (${CMAKE_CURRENT_BINARY_DIR}/Jinja2CppLight)

#        add_custom_target (
#                Jinja2CppLightPrebuild
#                COMMAND ${CMAKE_COMMAND} ARGS -G "${CMAKE_GENERATOR}" -D CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} ${CMAKE_CURRENT_SOURCE_DIR}/Jinja2CppLight
#                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Jinja2CppLight
#                COMMENT "Prebuild Jinja2CppLight library"
#        )
#else ()
#        add_custom_target (
#                Jinja2CppLightPrebuild
#        #	COMMAND ${CMAKE_COMMAND} ARGS -G "${CMAKE_GENERATOR}" -D CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} ${CMAKE_CURRENT_SOURCE_DIR}/Jinja2CppLight
#        #	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Jinja2CppLight
#                COMMENT "Prebuild Jinja2CppLight library"
#        )
#endif ()


#add_custom_target (
#        Jinja2CppLight
#        COMMAND ${CMAKE_COMMAND} --build .
#        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Jinja2CppLight
#        COMMENT "Build Jinja2CppLight library"
#        DEPENDS Jinja2CppLightPrebuild
#)

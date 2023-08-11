# Generate proto files
INCLUDE_DIRECTORIES(SYSTEM BEFORE PUBLIC ${PROTOBUF_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
if (EXISTS ${PROTOBUF_PROTOC_EXECUTABLE})
    message(STATUS "Found PROTOBUF Compiler: ${PROTOBUF_PROTOC_EXECUTABLE}")
else ()
    message(FATAL_ERROR "Could not find PROTOBUF Compiler")
endif ()
PROTOBUF_GENERATE_CPP(PLUGIN_INFO_PROTO_SRC PLUGIN_INFO_PROTO_H PluginInfo.proto)
PROTOBUF_GENERATE_PYTHON(PLUGIN_INFO_PROTO_PY PluginInfo.proto)
add_custom_target(generatePluginInfoProto
        ${CMAKE_COMMAND} -E copy_if_different
        ${PLUGIN_INFO_PROTO_PY} ${CMAKE_CURRENT_BINARY_DIR}/
        DEPENDS ${PLUGIN_INFO_PROTO_PY})

####

add_library(pluginregistryimpl SHARED
        PluginInfo.cpp PluginInfo.h
        Logger.cpp Logger.h
        ${PLUGIN_INFO_PROTO_SRC}
        ${PLUGIN_INFO_PROTO_H}
        PluginRegistryException.h
        ../UtilityImpl/StringUtils.h
        )
target_include_directories(pluginregistryimpl PUBLIC ${CMAKE_BINARY_DIR}/modules ${LOG4CPLUS_INCLUDE_DIR})
target_link_libraries(pluginregistryimpl
        utilityimpl
        filesystemimpl
        applicationconfigurationimpl
        processimpl
        protobufutil
        ${LOG4CPLUS_LIBRARY}
        )
add_dependencies(pluginregistryimpl generatePluginInfoProto)

SET_TARGET_PROPERTIES(pluginregistryimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS pluginregistryimpl
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)
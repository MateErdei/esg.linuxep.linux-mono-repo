include(proto/CMakeLists.txt)
add_library(pluginregistryimpl SHARED
        PluginInfo.cpp PluginInfo.h
        ${PLUGIN_INFO_PROTO_SRC}
        ${PLUGIN_INFO_PROTO_H} PluginRegistryException.h)
target_include_directories(pluginregistryimpl PUBLIC ../../Common)
target_link_libraries(pluginregistryimpl ${Protobuf_LIBRARIES} )
add_dependencies(pluginregistryimpl generatePluginInfoProto)
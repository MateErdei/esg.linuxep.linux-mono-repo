include(proto/CMakeLists.txt)
add_library(pluginregistryimpl SHARED
        PluginInfo.cpp PluginInfo.h
        Logger.h
        ${PLUGIN_INFO_PROTO_SRC}
        ${PLUGIN_INFO_PROTO_H}
        PluginRegistryException.h
        ../Datatypes/StringUtils.h)
target_include_directories(pluginregistryimpl PUBLIC ../../Common)
target_link_libraries(pluginregistryimpl ${Protobuf_LIBRARIES} utilityimpl filesystemimpl applicationconfigurationimpl)
add_dependencies(pluginregistryimpl generatePluginInfoProto)

install(TARGETS pluginregistryimpl
        LIBRARY DESTINATION files/base/lib64)
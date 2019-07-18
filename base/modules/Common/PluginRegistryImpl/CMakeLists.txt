include(proto/CMakeLists.txt)
add_library(pluginregistryimpl SHARED
        PluginInfo.cpp PluginInfo.h
        Logger.cpp Logger.h
        ${PLUGIN_INFO_PROTO_SRC}
        ${PLUGIN_INFO_PROTO_H}
        PluginRegistryException.h
        ../UtilityImpl/StringUtils.h
        )
target_include_directories(pluginregistryimpl PUBLIC ${LOG4CPLUS_INCLUDE_DIR})
target_link_libraries(pluginregistryimpl
            utilityimpl
            filesystemimpl
            applicationconfigurationimpl
            processimpl
            protobufutil
            ${LOG4CPLUS_LIBRARY}
        )
add_dependencies(pluginregistryimpl generatePluginInfoProto)

SET_TARGET_PROPERTIES( pluginregistryimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS pluginregistryimpl
        LIBRARY DESTINATION files/base/lib64)
add_library(responseactionsimpl SHARED
        ActionsUtils.cpp
        ActionsUtils.h
        ActionStructs.h
        DownloadFileAction.cpp
        DownloadFileAction.h
        Logger.cpp
        Logger.h
        InvalidCommandFormat.h
        UploadFileAction.cpp
        UploadFileAction.h
        UploadFolderAction.cpp
        UploadFolderAction.h
        RunCommandAction.cpp
        RunCommandAction.h
        )

include_directories(responseactionsimpl BEFORE
        ${CMAKE_CURRENT_BINARY_DIR}
        ${LOG4CPLUS_INCLUDE_DIR}
        ${NLOHMANN_JSON_INCLUDE_DIR}
        ${CMAKE_BINARY_DIR}/products/PluginApi/pluginapi/include
        )

target_link_libraries(responseactionsimpl PUBLIC
        ProxyUtilsLib
        HttpRequestsImpl
        curlwrapper
        ziputilities
        pluginapi
        ${Protobuf_LIBRARY}
        ${EXPAT_LIBRARY}
        ${ZeroMQ_LIBRARY}
        )

SET_TARGET_PROPERTIES(responseactionsimpl
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

install(TARGETS responseactionsimpl
        LIBRARY DESTINATION ${RA_DIST_PREFIX}/files/plugins/${RA_PLUGIN_NAME}/lib64)

add_dependencies(responseactionsimpl copy_libs)
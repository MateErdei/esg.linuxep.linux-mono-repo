add_library(responseactionsimpl SHARED
        ActionsUtils.cpp
        ActionsUtils.h
        ActionStructs.h
        IActionsImpl.h
        Logger.cpp
        Logger.h
        UploadFileAction.cpp
        UploadFileAction.h
)

include_directories(responseactionsimpl BEFORE ${CMAKE_CURRENT_BINARY_DIR} ${LOG4CPLUS_INCLUDE_DIR} ${CURL_INCLUDE_DIR} ${NLOHMANN_JSON_INCLUDE_DIR})

target_link_libraries(responseactionsimpl PUBLIC
        logging
        utilityimpl
        HttpRequestsImpl
        httpsenderimpl
        filesystemimplobject)

SET_TARGET_PROPERTIES(responseactionsimpl
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

install(TARGETS responseactionsimpl
        LIBRARY DESTINATION ${RA_DIST_PREFIX}/files/plugins/${RA_PLUGIN_NAME}/lib64)

add_dependencies(responseactionsimpl copy_libs)
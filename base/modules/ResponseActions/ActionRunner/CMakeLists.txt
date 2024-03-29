add_library(responseactionmain SHARED
        responseaction_main.cpp
        responseaction_main.h
        RunUtils.cpp
        RunUtils.h
        Logger.cpp
        Logger.h
        ../RACommon/ResponseActionsCommon.h
        ../RACommon/ResponseActionsCommon.cpp
        ../RACommon/toUtf8Exception.cpp
        ../RACommon/toUtf8Exception.h)
configure_file(config.h.in config.h)
target_include_directories(responseactionmain
        BEFORE PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
        ${LOG4CPLUS_INCLUDE_DIR}
        ${NLOHMANN_JSON_INCLUDE_DIR}
        ${BOOST_INCLUDE_DIR}
        )

target_link_libraries(responseactionmain PUBLIC
        pluginapi
        HttpRequestsImpl
        curlwrapper
        responseactionsimpl
        ${BOOST_LOCALE})

SET_TARGET_PROPERTIES(responseactionmain
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

install(TARGETS responseactionmain
        LIBRARY DESTINATION ${RA_DIST_PREFIX}/files/plugins/${RA_PLUGIN_NAME}/lib64)

add_dependencies(responseactionmain copy_libs)
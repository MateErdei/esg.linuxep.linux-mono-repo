sophos_add_exe(FuzzableResultSender
        FuzzableResultSender.cpp
        ../../../modules/osqueryextensions/IServiceExtension.h
        ../../../modules/osqueryextensions/Logger.h
        ../../../modules/osqueryextensions/Logger.cpp
        ../../../modules/osqueryextensions/ResultsSender.h
        ../../../modules/osqueryextensions/ResultsSender.cpp
        ../../../modules/osqueryextensions/SophosServerTable.h
        ../../../modules/osqueryextensions/SophosServerTable.cpp
        ../../../modules/osqueryextensions/SophosExtension.h
        ../../../modules/osqueryextensions/SophosExtension.cpp
        ${SQLITE_INCLUDE_DIR}/sqlite3.c
        EXTRA_LIBS
        ${log4cpluslib}
        ${protobuflib}
        ${OSQUERYSDK_LIBRARY}
        ${OSQUERY_GEN_LIBRARY}
        ${LOGGER_PLUGIN_LIBRARY}
        ${THRIFT_LIBRARY}
        ${JSON_CPP_LIBRARY}
        ${GFLAG_LIBRARY}
        ${GLOG_LIBRARY}
        ${THRIFT_GEN_LIBRARY}
        pthread
        EXTRA_INCLUDES ${OSQUERYSDK}/include ${SQLITE_INCLUDE_DIR} ${CMAKE_BINARY_DIR}  ${INPUT}/jsoncpp/include ${BOOST_INCLUDE_DIR}
        )

target_link_libraries(FuzzableResultSender PUBLIC
        ${GLOG_LIBRARY}
        ${GFLAG_LIBRARY}
        ${OSQUERYSDK_LIBRARY}
        ${OSQUERY_GEN_LIBRARY}
        ${THRIFT_LIBRARY}
        ${BOOST_CHRONO_LIBRARY}
        ${BOOST_SYSTEM_LIBRARY}
        ${BOOST_THREAD_LIBRARY}
        ${BOOST_FILESYSTEM_LIBRARY}
        ${BOOST_CONTEXT_LIBRARY}
        ${OPENSSLCRYPTO_LIBRARY}
        ${OPENSSL_LIBRARY}
        ${JSONCPP_LIBRARY}
        pthread
        ${CMAKE_DL_LIBS}
        dl
        )

target_include_directories(FuzzableResultSender PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}
        ${OSQUERYSDK}/include
        ${GOOGLETESTINCLUDE}
        ${BOOST_INCLUDE_DIR}
        ${SQLITE_INCLUDE_DIR}
        )

SET_TARGET_PROPERTIES(FuzzableResultSender
        PROPERTIES
        BUILD_RPATH "$ORIGIN:${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64"
        )

install(TARGETS FuzzableResultSender
        DESTINATION ../componenttests)

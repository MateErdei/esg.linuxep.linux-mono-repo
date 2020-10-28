sophos_add_library(serverinfotable SHARED
        SophosServerTable.cpp
        SophosServerTable.h
        ${SQLITE_INCLUDE_DIR}/sqlite3.c
        EXTRA_INCLUDES
            ${CMAKE_BINARY_DIR}
            ${CMAKE_SOURCE_DIR}/modules
            ${BOOST_INCLUDE_DIR}
            ${OSQUERYSDK}/include
            ${SQLITE_INCLUDE_DIR}
        EXTRA_LIBS
            ${GLOG_LIBRARY}
            ${GFLAG_LIBRARY}
            ${OSQUERY_LIBRARY}
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
            ${SQLITE_INCLUDE_DIR}
            ${log4cpluslib}
            ${protobuflib}
        )



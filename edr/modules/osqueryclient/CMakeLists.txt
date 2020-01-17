set( GOOGLETESTINCLUDE ${CMAKE_SOURCE_DIR}/tests/googletest/googletest/include )
message("gloglibrary: ${GLOG_LIBRARY}")
sophos_add_library(osqueryclient SHARED
        OsqueryProcessor.h
        OsqueryProcessor.cpp
        IOsqueryClient.h
        OsqueryClientImpl.h
        OsqueryClientImpl.cpp
        Logger.h
        Logger.cpp
        EXTRA_PROJECTS livequeryimpl
        EXTRA_INCLUDES
                ${CMAKE_CURRENT_BINARY_DIR}
                ${CMAKE_SOURCE_DIR}/modules
                ${OSQUERYSDK}/include
                ${GOOGLETESTINCLUDE}
                ${BOOST_INCLUDE_DIR}
        EXTRA_LIBS
            ${GLOG_LIBRARY}
            ${GFLAG_LIBRARY}
            ${OSQUERY_LIBRARY}
            ${THRIFT_LIBRARY}
            ${BOOST_CHRONO_LIBRARY}
            ${BOOST_SYSTEM_LIBRARY}
            ${BOOST_THREAD_LIBRARY}
            ${BOOST_FILESYSTEM_LIBRARY}
            ${BOOST_CONTEXT_LIBRARY}
            ${OPENSSLCRYPTO_LIBRARY}
            ${OPENSSL_LIBRARY}
        )

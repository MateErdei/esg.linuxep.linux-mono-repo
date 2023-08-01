set( GOOGLETESTINCLUDE ${CMAKE_SOURCE_DIR}/tests/googletest/googletest/include )

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
                ${GOOGLETESTINCLUDE}
                ${BOOST_INCLUDE_DIR}
                ${JSONCPP_INPUT}/include

        EXTRA_LIBS
            thrift_library
            ${JSONCPP_LIBRARY}
            osquerysdk_library
            sqlite_library
        )

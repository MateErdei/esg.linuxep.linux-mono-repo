configure_file(config.h.in config.h)

sophos_add_library(livequeryimpl SHARED
        IQueryProcessor.h
        QueryProcessor.cpp
        QueryResponse.h
        QueryResponse.cpp
        ResponseDispatcher.h
        ResponseDispatcher.cpp
        ResponseData.h
        ResponseData.cpp
        ResponseMetaData.h
        ResponseMetaData.cpp
        Logger.h
        Logger.cpp
        ${SQLITE_INCLUDE_DIR}/sqlite3.c
        EXTRA_PROJECTS queryrunnerimpl
        EXTRA_LIBS ${log4cpluslib} ${protobuflib} ${JSONCPP_LIBRARY} ${gflags} ${OPENSSL_LIBRARY} ${THRIFT_LIBRARY}
        EXTRA_INCLUDES ${OSQUERYSDK}/include
        )

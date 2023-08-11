
INCLUDE_DIRECTORIES(SYSTEM BEFORE PUBLIC ${PROTOBUF_INCLUDE_DIR})

include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(ENV{LD_LIBRARY_PATH} ${Protobuf_LD_LIBRARY_PATH})
#set(Protobuf_PROTOC_EXECUTABLE ${CMAKE_BINARY_DIR}/protoc.sh)

if (EXISTS ${PROTOBUF_PROTOC_EXECUTABLE})
        message(STATUS "Found PROTOBUF Compiler: ${PROTOBUF_PROTOC_EXECUTABLE}")
else ()
        message(FATAL_ERROR "Could not find PROTOBUF Compiler")
endif ()

PROTOBUF_GENERATE_CPP(DOWNLOADER_REPORT_PROTO_SRC DOWNLOADER_REPORT_PROTO_H DownloadReport.proto)

add_library(suldownloaderdataobject OBJECT
        ConfigurationData.cpp
        ConfigurationData.h
        ConfigurationDataUtil.h
        ConfigurationDataUtil.cpp
        DownloadedProduct.cpp
        DownloadedProduct.h
        TimeTracker.cpp
        TimeTracker.h
        RepositoryError.cpp
        RepositoryError.h
        SulDownloaderException.h
        DownloadReport.cpp
        DownloadReport.h
        Logger.cpp
        Logger.h
        ${CONFIG_SETTINGS_PROTO_SRC}
        ${CONFIG_SETTINGS_PROTO_H}
        ${DOWNLOADER_REPORT_PROTO_SRC}
        ${DOWNLOADER_REPORT_PROTO_H}
        ConnectionSetup.cpp
        ConnectionSetup.h
        ProductMetadata.cpp
        ProductMetadata.h
        ConnectionSelector.cpp
        ConnectionSelector.h
        Tag.h
        IVersig.h
        VersigImpl.cpp
        VersigImpl.h
        CatalogueInfo.cpp
        CatalogueInfo.h
        UpdateSupplementDecider.cpp
        UpdateSupplementDecider.h
        ISdds3Repository.h
        )

target_include_directories(suldownloaderdataobject PUBLIC
        ${PROJECT_SOURCE_DIR}/modules
        ${CMAKE_BINARY_DIR}/modules
        ${LOG4CPLUS_INCLUDE_DIR}
        ${NLOHMANN_JSON_INCLUDE_DIR}
        ${SOPHLIB_INCLUDE}
        )

add_library(suldownloaderdata SHARED)

target_link_libraries(suldownloaderdata PRIVATE
        suldownloaderdataobject
        filesystemimpl
        processimpl
        threads
        utilityimpl
        applicationconfigurationimpl
        logging
        obfuscationimpl
        protobufutil
        ProxyUtilsLib
        )

install(TARGETS suldownloaderdata
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)

include(proto/CMakeLists.txt)

add_library(suldownloaderdataobject OBJECT
        ConfigurationData.cpp
        ConfigurationData.h
        ConfigurationDataUtil.h
        ConfigurationDataUtil.cpp
        Credentials.cpp
        Credentials.h
        Proxy.cpp
        Proxy.h
        DownloadedProduct.cpp
        DownloadedProduct.h
        TimeTracker.cpp
        TimeTracker.h
        RepositoryError.cpp
        RepositoryError.h
        SulDownloaderException.h
        SulDownloaderException.cpp
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
        ProductSelection.cpp
        ProductSelection.h
        Tag.h
        IVersig.h
        VersigImpl.cpp
        VersigImpl.h
        CatalogueInfo.cpp
        CatalogueInfo.h
        WeekDayAndTimeForDelay.h
        UpdateSupplementDecider.cpp
        UpdateSupplementDecider.h
        ISdds3Repository.h
        )

target_include_directories(suldownloaderdataobject PUBLIC
        ${PROJECT_SOURCE_DIR}/modules
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

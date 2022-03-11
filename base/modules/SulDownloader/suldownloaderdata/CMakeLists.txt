include(proto/CMakeLists.txt)

add_library(suldownloaderdata SHARED
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
target_include_directories(suldownloaderdata PUBLIC  ../../../thirdparty/nlohmann-json)
target_link_libraries(suldownloaderdata PUBLIC filesystemimpl processimpl threads utilityimpl applicationconfigurationimpl logging obfuscationimpl protobufutil commscomponentimpl)
install(TARGETS suldownloaderdata
        LIBRARY DESTINATION files/base/lib64)

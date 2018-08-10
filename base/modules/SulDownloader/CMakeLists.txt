include(proto/CMakeLists.txt)

add_library(suldownloaderdata SHARED
        ConfigurationData.cpp
        ConfigurationData.h
        Credentials.cpp
        Credentials.h
        Proxy.cpp
        Proxy.h
        DownloadedProduct.cpp
        DownloadedProduct.h
        TimeTracker.cpp
        TimeTracker.h
        WarehouseError.cpp
        WarehouseError.h
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
        VersigImpl.h)

add_library(suldownloaderimpl SHARED
    SULRaii.h
    SULUtils.cpp
    SULUtils.h
    WarehouseRepository.cpp
    WarehouseRepository.h
    WarehouseRepositoryFactory.cpp
    WarehouseRepositoryFactory.h
    IWarehouseRepository.h
    SULDownloader.cpp
    SulDownloader.h

)


set (CMAKE_EXE_LINKER_FLAGS -Wl,-rpath,${SUL_LIBRARY_DIRS} )
set( SUL_LIBRARY -L${SUL_LIBRARY_DIR}  -lSUL)

target_link_libraries(suldownloaderdata PUBLIC ${Protobuf_LIBRARIES} filesystemimpl processimpl threads utilityimpl applicationconfigurationimpl)

target_link_libraries(suldownloaderimpl PUBLIC suldownloaderdata ${SUL_LIBRARY} )


include_directories(BEFORE ${SUL_INPUT}/include)

SET_TARGET_PROPERTIES( suldownloaderimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")


install(TARGETS suldownloaderdata
        LIBRARY DESTINATION files/base/lib64)


install(TARGETS suldownloaderimpl
        LIBRARY DESTINATION files/base/lib64)

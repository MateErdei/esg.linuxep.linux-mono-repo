include(proto/CMakeLists.txt)
add_library(suldownloaderimpl SHARED
        SULDownloader.cpp
        SulDownloader.h
        DownloadedProduct.cpp
        DownloadedProduct.h
        ConfigurationData.cpp
        ConfigurationData.h
        ConnectionSetup.cpp
        ConnectionSetup.h
        ProductMetadata.cpp
        ProductMetadata.h
        Credentials.cpp
        Credentials.h
        Proxy.cpp
        Proxy.h
        SULRaii.h
        SulDownloaderException.h
        SulDownloaderException.cpp
        ConnectionSelector.cpp
        ConnectionSelector.h
        WarehouseRepository.cpp
        WarehouseRepository.h
        ProductSelection.cpp
        ProductSelection.h
        Tag.h
        DownloadReport.cpp
        DownloadReport.h
        SULUtils.cpp
        SULUtils.h
        Logger.cpp
        Logger.h
        ${CONFIG_SETTINGS_PROTO_SRC}
        ${CONFIG_SETTINGS_PROTO_H}
        ${DOWNLOADER_REPORT_PROTO_SRC}
        ${DOWNLOADER_REPORT_PROTO_H}
        TimeTracker.cpp
        TimeTracker.h
        WarehouseError.cpp
        WarehouseError.h
        WarehouseRepositoryFactory.cpp
        WarehouseRepositoryFactory.h
        IWarehouseRepository.h
        IVersig.h
        VersigImpl.cpp
        VersigImpl.h)

set (CMAKE_EXE_LINKER_FLAGS -Wl,-rpath,${SUL_LIBRARY_DIRS} )
set( SUL_LIBRARY -L${SUL_LIBRARY_DIR}  -lSUL)

target_link_libraries(suldownloaderimpl PUBLIC ${SUL_LIBRARY} ${Protobuf_LIBRARIES} filesystemimpl processimpl threads utilityimpl applicationconfigurationimpl)

include_directories(BEFORE ${SUL_INPUT}/include)

SET_TARGET_PROPERTIES( suldownloaderimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS suldownloaderimpl
        LIBRARY DESTINATION files/base/lib64)

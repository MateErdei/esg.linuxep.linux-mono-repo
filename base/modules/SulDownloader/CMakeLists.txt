include(proto/CMakeLists.txt)
add_library(SulDownloader SHARED
        SULDownloader.cpp
        SulDownloader.h
        Product.cpp
        Product.h
        ConfigurationData.cpp
        ConfigurationData.h
        ConnectionSetup.cpp
        ConnectionSetup.h
        ProductInformation.cpp
        ProductInformation.h
        Credentials.cpp
        Credentials.h
        Proxy.cpp
        Proxy.h
        SULRaii.h
        SulDownloaderException.h
        SulDownloaderException.cpp
        ConnectionSelector.cpp
        ConnectionSelector.h
        Warehouse.cpp
        Warehouse.h
        ProductSelection.cpp
        ProductSelection.h
        Tag.h
        DownloadReport.cpp
        DownloadReport.h
        SULUtils.cpp
        SULUtils.h
        Logger.cpp
        Logger.h
        MessageUtility.cpp
        MessageUtility.h
        ${CONFIG_SETTINGS_PROTO_SRC}
        ${CONFIG_SETTINGS_PROTO_H}
        ${DOWNLOADER_REPORT_PROTO_SRC}
        ${DOWNLOADER_REPORT_PROTO_H}
        TimeTracker.cpp TimeTracker.h WarehouseError.cpp WarehouseError.h)

#
#/redist/binaries/linux10/input/SUL
set( REDIST /redist )
set( BINARIES_DIR ${REDIST}/binaries )
set( INPUT ${BINARIES_DIR}/linux10/input )
set( SUL_INPUT ${INPUT}/SUL )
set( BOOST_INPUT ${INPUT}/boost )
set( SUL_LIBRARY_DIR ${SUL_INPUT}/lib64 )
set( SUL_LIBRARY_DIRS ${SUL_LIBRARY_DIR}:${BOOST_INPUT}/lib64)

set (CMAKE_EXE_LINKER_FLAGS -Wl,-rpath,${SUL_LIBRARY_DIRS} )
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall")

set( SUL_LIBRARY -L${SUL_LIBRARY_DIR}  -lSUL)

target_link_libraries(SulDownloader PUBLIC ${SUL_LIBRARY} ${Protobuf_LIBRARIES} filesystemimpl)

include_directories(BEFORE ${SUL_INPUT}/include)
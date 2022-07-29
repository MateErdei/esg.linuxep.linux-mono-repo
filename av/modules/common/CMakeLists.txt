add_library(common SHARED
        AbortScanException.h
        ErrorCodes.h
        ErrorCodesC.h
        Logger.cpp
        Logger.h
        ShuttingDownException.h
        SigHupMonitor.cpp
        SigHupMonitor.h
        SigIntMonitor.cpp
        SigIntMonitor.h
        SigTermMonitor.cpp
        SigTermMonitor.h
        StringUtils.cpp
        StringUtils.h
        ThreadRunner.cpp
        ThreadRunner.h
        PathUtils.cpp
        PathUtils.h
        PluginUtils.cpp
        PluginUtils.h
        ScanInterruptedException.h
        ScanManuallyInterruptedException.h)

SET (BOOST_LIBRARYDIR  ${BOOSTPATH}/boost/lib64)
SET (BOOST_INCLUDEDIR ${BOOSTPATH}/boost/include)
SET (BOOST_ROOT ${BOOSTPATH}/boost)

find_package( Boost REQUIRED COMPONENTS filesystem locale system)

target_link_libraries(common  PUBLIC
        scanmessages
        datatypes
        ${STD_FILESYSTEM_IF_REQUIRED}
        ${pluginapilib}
        ${CRYPTO_LIBRARY}
        ${Boost_LIBRARIES}
        )

target_include_directories(common PUBLIC ${pluginapiinclude} ${CMAKE_SOURCE_DIR}/modules ${OPENSSL_INCLUDE_DIR})
target_include_directories(common SYSTEM PUBLIC ${BOOST_INCLUDEDIR})

install(TARGETS common
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

SET_TARGET_PROPERTIES(common
        PROPERTIES
        BUILD_RPATH "$ORIGIN:${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "$ORIGIN"
        )
configure_file(config.h.in config.h )
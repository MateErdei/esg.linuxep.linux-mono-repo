add_library(common SHARED
        AbortScanException.h
        ApplicationPaths.cpp
        ApplicationPaths.h
        CentralEnums.h
        ErrorCodes.h
        ErrorCodesC.h
        Exclusion.cpp
        Exclusion.h
        FDUtils.cpp
        FDUtils.h
        IPidLockFile.h
        LockableData.h
        Logger.cpp
        Logger.h
        NotifyPipeSleeper.cpp
        NotifyPipeSleeper.h
        PathUtils.cpp
        PathUtils.h
        PidLockFile.cpp
        PidLockFile.h
        PidLockFileException.h
        PluginUtils.cpp
        PluginUtils.h
        SaferStrerror.cpp
        SaferStrerror.h
        ScanInterruptedException.h
        ScanManuallyInterruptedException.h
        StoppableSleeper.h
        ShuttingDownException.h
        StringUtils.cpp
        StringUtils.h
        ThreadRunner.cpp
        ThreadRunner.h
        signals/IReloadable.h
        signals/SigHupMonitor.cpp
        signals/SigHupMonitor.h
        signals/SigIntMonitor.cpp
        signals/SigIntMonitor.h
        signals/LatchingSignalHandler.cpp
        signals/LatchingSignalHandler.h
        signals/SignalHandlerBase.cpp
        signals/SignalHandlerBase.h
        signals/SignalHandlerTemplate.h
        signals/SigTermMonitor.cpp
        signals/SigTermMonitor.h
        signals/SigUSR1Monitor.cpp
        signals/SigUSR1Monitor.h)

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
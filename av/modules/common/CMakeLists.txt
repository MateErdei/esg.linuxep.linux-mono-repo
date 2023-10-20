add_library(common SHARED
        AbortScanException.h
        ApplicationPaths.cpp
        ApplicationPaths.h
        CentralEnums.h
        ErrorCodes.h
        ErrorCodesC.h
        Exclusion.cpp
        Exclusion.h
        FailedToInitializeSusiException.h
        FDUtils.cpp
        FDUtils.h
        InotifyFD.cpp
        InotifyFD.h
        IPidLockFile.h
        LockableData.h
        Logger.cpp
        Logger.h
        MultiInotifyFD.cpp
        MultiInotifyFD.h
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
        ShuttingDownException.h
        StatusFile.cpp
        StatusFile.h
        StoppableSleeper.h
        StringUtils.cpp
        StringUtils.h
        ThreadRunner.cpp
        ThreadRunner.h
        UsernameSetting.h
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
        signals/SigUSR1Monitor.h
        ThreatDetector/SusiSettings.cpp
        ThreatDetector/SusiSettings.h
        )

target_link_libraries(common  PUBLIC
        datatypes
        ${STD_FILESYSTEM_IF_REQUIRED}
        ${pluginapilib}
        boost_locale
        )

target_include_directories(common PUBLIC
        ${pluginapiinclude}
        ${CMAKE_SOURCE_DIR}/modules
        ${JSON_INPUT}
        ${JSON_SINGLE_INPUT}
)
target_include_directories(common SYSTEM PUBLIC ${BOOST_INCLUDEDIR})

install(TARGETS common
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

SET_TARGET_PROPERTIES(common
        PROPERTIES
        BUILD_RPATH "$ORIGIN:${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "$ORIGIN"
        )
configure_file(config.h.in config.h )
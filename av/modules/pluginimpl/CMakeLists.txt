

add_subdirectory(ObfuscationImpl)

sophos_add_library(pluginimpl SHARED
        HealthStatus.cpp
        HealthStatus.h
        IDetectionReportProcessor.h
        PolicyProcessor.cpp
        PolicyProcessor.h
        PolicyWaiter.cpp
        PolicyWaiter.h
        PluginCallback.cpp
        PluginCallback.h
        QueueTask.cpp
        QueueTask.h
        QueueSafeStoreTask.cpp
        QueueSafeStoreTask.h
        Logger.h
        Logger.cpp
        PluginAdapter.cpp
        PluginAdapter.h
        SafeStoreWorker.cpp
        SafeStoreWorker.h
        StringUtils.cpp
        StringUtils.h
        ../common/config.h
        EXTRA_INCLUDES ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/modules
        EXTRA_LIBS ${zmqlib} ${log4cpluslib} ${protobuflib} scanscheduler unixsocket scanprocessmonitor obfuscationimpl
)

target_include_directories(pluginimpl PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

SET_TARGET_PROPERTIES(pluginimpl
        PROPERTIES
        BUILD_RPATH "$ORIGIN:${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../../../base/lib64"
        )
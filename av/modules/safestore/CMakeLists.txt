
add_library(safestoreimpl SHARED
        Logger.cpp
        Logger.h
        Main.cpp
        Main.h
        QuarantineManager/IQuarantineManager.h
        QuarantineManager/QuarantineManagerImpl.cpp
        QuarantineManager/QuarantineManagerImpl.h
        QuarantineManager/StateMonitor.cpp
        QuarantineManager/StateMonitor.h
        SafeStoreServiceCallback.cpp
        SafeStoreServiceCallback.h
        SafeStoreTelemetryConsts.h
        SafeStoreWrapper/ISafeStoreWrapper.h
        SafeStoreWrapper/SafeStoreWrapperImpl.cpp
        SafeStoreWrapper/SafeStoreWrapperImpl.h
        )

target_link_libraries(safestoreimpl PUBLIC
        ${SAFESTORE_LIB}
        ${log4cpluslib}
        common
        unixsocket
        )

target_include_directories(safestoreimpl PUBLIC ${SAFESTORE_INPUT})


install(TARGETS safestoreimpl
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

SET_TARGET_PROPERTIES(safestoreimpl
        PROPERTIES
        BUILD_RPATH "$ORIGIN:${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../../../base/lib64:/opt/sophos-spl/plugins/av/lib64:/opt/sophos-spl/base/lib64"
        )

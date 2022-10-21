
add_library(safestoreimpl SHARED
        SafeStoreWrapper/ISafeStoreWrapper.h
        QuarantineManager/IQuarantineManager.h
        Logger.cpp
        Logger.h
        Main.cpp
        Main.h
        QuarantineManager/QuarantineManagerImpl.h
        QuarantineManager/QuarantineManagerImpl.cpp
        SafeStoreWrapper/SafeStoreWrapperImpl.h
        SafeStoreWrapper/SafeStoreWrapperImpl.cpp
        QuarantineManager/StateMonitor.cpp
        QuarantineManager/StateMonitor.h
        )

target_link_libraries(safestoreimpl PUBLIC
        ${SAFESTORE_LIB}
        common
        unixsocket
        ${log4cpluslib}
        )

target_include_directories(safestoreimpl PUBLIC ${SAFESTORE_INPUT})


install(TARGETS safestoreimpl
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

SET_TARGET_PROPERTIES(safestoreimpl
        PROPERTIES
        BUILD_RPATH "$ORIGIN:${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../../../base/lib64:/opt/sophos-spl/plugins/av/lib64:/opt/sophos-spl/base/lib64"
        )


add_library(safestoreimpl SHARED
        ISafeStoreResources.h
        Logger.cpp
        Logger.h
        Main.cpp
        Main.h
        QuarantineManager/IQuarantineManager.h
        QuarantineManager/QuarantineManagerImpl.cpp
        QuarantineManager/QuarantineManagerImpl.h
        QuarantineManager/StateMonitor.cpp
        QuarantineManager/StateMonitor.h
        SafeStoreResources.cpp
        SafeStoreResources.h
        SafeStoreServiceCallback.cpp
        SafeStoreServiceCallback.h
        SafeStoreTelemetryConsts.h
        SafeStoreWrapper/ISafeStoreWrapper.h
        SafeStoreWrapper/SafeStoreWrapperImpl.cpp
        SafeStoreWrapper/SafeStoreWrapperImpl.h
        )

add_dependencies(safestoreimpl copy_libs)

target_include_directories(safestoreimpl PUBLIC ${CMAKE_SOURCE_DIR}/products/distribution/include)

target_link_libraries(safestoreimpl PUBLIC
        safestorelib
        ${log4cpluslib}
        common
        unixsocket
        )


install(TARGETS safestoreimpl
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

SET_TARGET_PROPERTIES(safestoreimpl
        PROPERTIES
        BUILD_RPATH "$ORIGIN:${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "$ORIGIN"
        )

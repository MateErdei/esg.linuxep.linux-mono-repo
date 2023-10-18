add_library(remotediagnoseimpl SHARED
        Main.cpp
        Main.h
        PluginCallback.cpp
        PluginCallback.h
        PluginAdapter.cpp
        PluginAdapter.h
        PluginUtils.cpp
        PluginUtils.h
        taskQueue/TaskQueue.cpp
        taskQueue/TaskQueue.h
        taskQueue/ITaskQueue.h
        logger/Logger.cpp
        logger/Logger.h
        runnerModule/IAsyncDiagnoseRunner.h
        runnerModule/AsyncDiagnoseRunner.cpp
        runnerModule/AsyncDiagnoseRunner.h
        runnerModule/DiagnoseResultDirectoryListener.h
        runnerModule/DiagnoseResultDirectoryListener.cpp
        runnerModule/DiagnoseRunner.cpp
        runnerModule/DiagnoseRunner.h
        runnerModule/DiagnoseRunnerFactory.cpp
        runnerModule/DiagnoseRunnerFactory.h
        )

include_directories(remotediagnoseimpl BEFORE
        ${LOG4CPLUS_INCLUDE_DIR}
        ${NLOHMANN_JSON_INCLUDE_DIR}
        ${AUTOVERSION_INCLUDE}
)

target_link_libraries(remotediagnoseimpl PUBLIC
        applicationconfigurationimpl
        curlwrapper
        directorywatcherimpl
        HttpRequestsImpl
        osutilitiesimpl
        pluginapiimpl
        processimpl
        utilityimpl
        watchdogservice
        xmlutilities
        )

SET_TARGET_PROPERTIES(remotediagnoseimpl
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

install(TARGETS remotediagnoseimpl
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)

add_dependencies(remotediagnoseimpl copy_libs)

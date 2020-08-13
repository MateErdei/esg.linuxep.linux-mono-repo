add_library(avscannerimpl SHARED
        BaseFileWalkCallbacks.cpp
        BaseFileWalkCallbacks.h
        BaseRunner.cpp
        BaseRunner.h
        CommandLineScanRunner.cpp
        CommandLineScanRunner.h
        Exclusion.cpp
        Exclusion.h
        Logger.cpp
        Logger.h
        IRunner.h
        ISystemPathsFactory.h
        NamedScanConfig.cpp
        NamedScanConfig.h
        NamedScanRunner.cpp
        NamedScanRunner.h
        Options.cpp
        Options.h
        PathUtils.cpp
        PathUtils.h
        ScanClient.cpp
        ScanClient.h
        ../mountinfoimpl/DeviceUtil.cpp
        ../mountinfoimpl/DeviceUtil.h
        ../mountinfoimpl/ISystemCallWrapper.h
        ../mountinfoimpl/ISystemCallWrapperFactory.h
        ../mountinfoimpl/Mounts.cpp
        ../mountinfoimpl/Mounts.h
        ../mountinfoimpl/SystemCallWrapper.h
        ../mountinfoimpl/SystemCallWrapperFactory.cpp
        ../mountinfoimpl/SystemCallWrapperFactory.h
        ../mountinfoimpl/SystemPaths.h
        ../mountinfoimpl/SystemPathsFactory.cpp
        ../mountinfoimpl/SystemPathsFactory.h
        ../mountinfo/IMountInfo.h
        ../mountinfo/IMountPoint.h
        ../mountinfo/ISystemPaths.h
        )

SET (BOOST_LIBRARYDIR  ${BOOSTPATH}/boost/lib64)
SET (BOOST_INCLUDEDIR ${BOOSTPATH}/boost/include)
SET (BOOST_ROOT ${BOOSTPATH}/boost)

find_package( Boost REQUIRED COMPONENTS program_options)

target_include_directories(avscannerimpl PUBLIC
        ${CMAKE_SOURCE_DIR}/modules
        "${CAPNPROTO_INCLUDE_DIR}"
        ${CMAKE_BINARY_DIR}/modules/scan_messages
        ${CMAKE_CURRENT_BINARY_DIR}
        ${BOOSTPATH})

target_link_libraries(avscannerimpl
        datatypes
        scanmessages
        unixsocket
        filewalker
        $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>>:stdc++fs>
        ${Boost_LIBRARIES})

install(TARGETS avscannerimpl
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

SET_TARGET_PROPERTIES(avscannerimpl
        PROPERTIES
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../../../base/lib64"
        BUILD_RPATH "$ORIGIN")

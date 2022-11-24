
SophosAddTest(TestSafeStore
        ../common/LogInitializedTests.cpp # include here as it's not part of a lib
        MockIQuarantineManager.h
        MockISafeStoreWrapper.h
        TestQuarantineManager.cpp
        TestQuarantineManagerRescan.cpp
        TestSafeStoreServiceCallback.cpp
        TestSafeStoreWrapperUtils.cpp
        TestStateMonitor.cpp
        PROJECTS safestoreimpl
        INC_DIRS
        ${testhelpersinclude}
        LIBS ${log4cpluslib} ${testhelperslib}
        )

#----------------------------------------------

# Binary to be run in TAP tests
add_executable(SafeStoreTapTests
        SafeStoreTapTests.cpp
        ../common/LogInitializedTests.cpp
        )

target_include_directories(SafeStoreTapTests SYSTEM BEFORE PUBLIC
        ${CMAKE_SOURCE_DIR}/modules
        ${GMOCK_INCLUDE}
        ${GTEST_INCLUDE}
        )

target_link_libraries(SafeStoreTapTests
        ${GTEST_LIBRARY}
        ${GTEST_MAIN_LIBRARY}
        ${testhelperslib}
        ${log4cpluslib}
        ${pluginapilib}
        safestoreimpl
        pthread
        )
set_target_properties(SafeStoreTapTests PROPERTIES
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        # Will rely on having AV installed to run this binary so we don't need to keep a list of needed libs up to date.
        INSTALL_RPATH "$ORIGIN:/opt/sophos-spl/plugins/av/lib64/")

install(TARGETS SafeStoreTapTests
        DESTINATION ${CMAKE_BINARY_DIR}/tap_test_output)

add_dependencies(SafeStoreTapTests copy_libs)

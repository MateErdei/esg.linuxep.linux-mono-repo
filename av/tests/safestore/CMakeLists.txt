
include(GoogleTest)

SophosAddTest(TestSafeStore
        ../common/LogInitializedTests.cpp # include here as it's not part of a lib
        MockISafeStoreWrapper.h
        TestQuarantineManager.cpp
        TestSafeStoreWrapperUtils.cpp
        TestStateMonitor.cpp
        PROJECTS safestoreimpl common
        INC_DIRS ${pluginapiinclude} ${CMAKE_SOURCE_DIR}/modules ${testhelpersinclude}
        LIBS pthread ${log4cpluslib} ${testhelperslib} ${pluginapilb}
        )

SET_TARGET_PROPERTIES(TestSafeStore
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
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

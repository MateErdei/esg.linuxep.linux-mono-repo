
include(GoogleTest)

SophosAddTest(TestSafeStore
        MockISafeStoreWrapper.h
        TestQuarantineManager.cpp
        TestSafeStoreWrapper.cpp
        ../common/LogInitializedTests.cpp # include here as it's not part of a lib
        PROJECTS safestoreimpl common
        INC_DIRS ${pluginapiinclude} ${CMAKE_SOURCE_DIR}/modules
        LIBS pthread ${log4cpluslib} ${testhelperslib}
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
        INSTALL_RPATH "/opt/sophos-spl/base/lib64/")

install(TARGETS SafeStoreTapTests
        DESTINATION ${CMAKE_BINARY_DIR}/tap_test_output)

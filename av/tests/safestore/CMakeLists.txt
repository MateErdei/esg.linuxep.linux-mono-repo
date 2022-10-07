
include(GoogleTest)

SophosAddTest(TestSafeStore
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
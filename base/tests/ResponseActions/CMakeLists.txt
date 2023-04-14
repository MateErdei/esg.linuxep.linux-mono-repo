include(GoogleTest)

add_executable(ResponseActionsTests
        TestActionsUtils.cpp
        TestDownloadAction.cpp
        TestPluginAdapter.cpp
        TestPluginUtils.cpp
        TestUploadAction.cpp
        TestUploadFolderAction.cpp
        TestRunCommand.cpp
)

target_include_directories(ResponseActionsTests SYSTEM BEFORE PUBLIC "${GTEST_INCLUDE}" "${GMOCK_INCLUDE}" )
target_include_directories(ResponseActionsTests BEFORE PUBLIC
        "${PROJECT_SOURCE_DIR}"
        ${NLOHMANN_JSON_INCLUDE_DIR}
        ${CMAKE_BINARY_DIR}/products/PluginApi/tests/include
        )

target_link_libraries(ResponseActionsTests ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY} -L"${CMAKE_BINARY_DIR}/libs"
        responseplugin
        responseactionsimpl
        sharedtesthelpers
        )

gtest_discover_tests(ResponseActionsTests)

SET_TARGET_PROPERTIES(ResponseActionsTests
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
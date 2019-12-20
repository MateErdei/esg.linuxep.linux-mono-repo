
SophosAddTest(TestLiveQuery
        TestQueryProcessor.cpp
        PROJECTS livequeryimpl
        INC_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${testhelpersinclude}
        )

SET_TARGET_PROPERTIES(TestLiveQuery
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

SophosAddTest(TestLiveQuery
        TestQueryProcessor.cpp
        TestResponseData.cpp
        MockQueryProcessor.h
        PROJECTS livequeryimpl
        LIBS ${log4cpluslib} gmock gtest
        INC_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${testhelpersinclude}
        )

add_dependencies(TestLiveQuery copy_libs)

SET_TARGET_PROPERTIES(TestLiveQuery
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
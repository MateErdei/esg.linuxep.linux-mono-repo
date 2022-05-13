
SophosAddTest(TestLiveQuery
        TestQueryProcessor.cpp
        TestResponseData.cpp
        TestResponseDispatcher.cpp
        TestLiveQueryTelemetry.cpp
        MockQueryProcessor.h
        PROJECTS livequeryimpl
        LIBS ${log4cpluslib} ${pluginapilib} ${testhelperslib} pthread
        INC_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${testhelpersinclude}
        )

add_dependencies(TestLiveQuery copy_libs)

SET_TARGET_PROPERTIES(TestLiveQuery
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
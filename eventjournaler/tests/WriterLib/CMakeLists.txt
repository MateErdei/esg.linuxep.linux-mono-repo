include(GoogleTest)

SophosAddTest(TestWriterLib
        TestEventQueuePopper.cpp
        TestWriter.cpp
        PROJECTS writerlib eventqueuelib
        LIBS ${log4cpluslib} ${pluginapilib} ${testhelperslib} gmock gtest ${protobuflib}
        INC_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${testhelpersinclude}
        )

add_dependencies(TestWriterLib copy_libs)
SET_TARGET_PROPERTIES(TestWriterLib
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

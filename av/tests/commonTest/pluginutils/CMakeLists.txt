SophosAddTest(TestPluginUtils
        ../../common/LogInitializedTests.cpp
        TestThreadRunner.cpp
        PROJECTS common
        INC_DIRS
        ${CMAKE_SOURCE_DIR}/modules
        ${CMAKE_SOURCE_DIR}/tests
        ${CMAKE_SOURCE_DIR}
        ${pluginapiinclude}
        LIBS
        ${log4cpluslib}
        ${pluginapilib}
        )

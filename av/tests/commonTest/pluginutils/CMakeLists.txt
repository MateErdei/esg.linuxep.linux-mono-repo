SophosAddTest(TestPluginUtils
        ../../common/LogInitializedTests.cpp
        TestThreadRunner.cpp
        PROJECTS common
        INC_DIRS
        ${pluginapiinclude}
        LIBS
        ${log4cpluslib}
        ${pluginapilib}
        )

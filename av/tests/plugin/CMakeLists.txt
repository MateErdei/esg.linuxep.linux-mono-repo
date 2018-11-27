include(GoogleTest)

SophosAddTest(TestPlugin
        TestPlugin.cpp
        PROJECTS ${pluginapilib}
        LIBS ${log4cpluslib}
        INC_DIRS ${pluginapiinclude} )
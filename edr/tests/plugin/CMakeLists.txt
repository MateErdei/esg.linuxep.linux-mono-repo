include(GoogleTest)

SophosAddTest(TestPlugin
        TestPlugin.cpp
        PROJECTS pluginimpl
)

# example with third party dependencies
#SophosAddTest(TestPlugin
#        TestPlugin.cpp
#        PROJECTS pluginimpl
#        LIBS ${log4cpluslib} ${pluginapilib}
#        INC_DIRS ${pluginapiinclude} )
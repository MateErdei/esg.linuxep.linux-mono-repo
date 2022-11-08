
add_subdirectory(ObfuscationImpl)

SophosAddTest(TestPlugin
        ../common/LogInitializedTests.cpp
        TestActionUtils.cpp
        TestDetectionQueue.cpp
        TestPolicyProcessor.cpp
        TestPlugin.cpp
        TestPluginAdapter.cpp
        TestPluginCallback.cpp
        TestPolicyWaiter.cpp
        TestStringUtils.cpp
        TestTaskQueue.cpp
        TestThreatDatabase.cpp
        PROJECTS pluginimpl
        LIBS ${pluginapilib} ${testhelperslib}
        INC_DIRS ${pluginapiinclude} ${testhelpersinclude}
        )

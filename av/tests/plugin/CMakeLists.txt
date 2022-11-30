
add_subdirectory(ObfuscationImpl)

SophosAddTest(TestPlugin
        ../common/LogInitializedTests.cpp
        TestActionUtils.cpp
        TestDetectionQueue.cpp
        TestPolicyProcessor.cpp
        TestPolicyProcessor.h
        TestPolicyProcessor_ALC_policy.cpp
        TestPolicyProcessor_CORC_policy.cpp
        TestPolicyProcessor_CORE_policy.cpp
        TestPolicyProcessor_FLAGS_policy.cpp
        TestPolicyProcessor_SAV_policy.cpp
        TestPlugin.cpp
        TestPluginAdapter.cpp
        TestPluginCallback.cpp
        TestPolicyWaiter.cpp
        TestStringUtils.cpp
        TestTaskQueue.cpp
        TestThreatDatabase.cpp
        PROJECTS pluginimpl fanotifyhandler
        LIBS ${pluginapilib} ${testhelperslib}
        INC_DIRS ${pluginapiinclude} ${testhelpersinclude}
        )

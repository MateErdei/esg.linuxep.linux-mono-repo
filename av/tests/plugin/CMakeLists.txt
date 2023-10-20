if (NOT DEFINED ENV{USE_LIBFUZZER})

    SophosAddTest(TestPlugin
            ../common/LogInitializedTests.cpp
            MockDetectionHandler.h
            TestActionUtils.cpp
            TestDetectionQueue.cpp
            TestHealthStatus.cpp
            TestOnAccessStatusMonitor.cpp
            TestPolicyProcessor.cpp
            TestPolicyProcessor.h
            TestPolicyProcessor_ALC_policy.cpp
            TestPolicyProcessor_CORC_policy.cpp
            TestPolicyProcessor_CORE_policy.cpp
            TestPolicyProcessor_FLAGS_policy.cpp
            TestPolicyProcessor_SAV_policy.cpp
            TestPlugin.cpp
            TestPluginAdapter.cpp
            TestPluginAdapterDetections.cpp
            TestPluginCallback.cpp
            TestPolicyWaiter.cpp
            TestSafeStoreWorker.cpp
            TestStringUtils.cpp
            TestTaskQueue.cpp
            TestTelemetry.cpp
            TestThreatDatabase.cpp
            PROJECTS pluginimpl fanotifyhandler
            LIBS ${pluginapilib} ${testhelperslib}
            INC_DIRS ${pluginapiinclude} ${testhelpersinclude}
            )
endif (NOT DEFINED ENV{USE_LIBFUZZER})

add_executable(CorcPolicyProcessorFuzzer
        CorcPolicyProcessorFuzzer.cpp)
add_executable(CorePolicyProcessorFuzzer
        CorePolicyProcessorFuzzer.cpp)

target_link_libraries(CorcPolicyProcessorFuzzer
        pluginimpl)
target_include_directories(CorcPolicyProcessorFuzzer PUBLIC ${CMAKE_SOURCE_DIR}/modules INC_DIRS ${pluginapiinclude} )

target_link_libraries(CorePolicyProcessorFuzzer
        pluginimpl)
target_include_directories(CorePolicyProcessorFuzzer PUBLIC ${CMAKE_SOURCE_DIR}/modules INC_DIRS ${pluginapiinclude} )

if (DEFINED ENV{USE_LIBFUZZER})
    SET_TARGET_PROPERTIES( CorcPolicyProcessorFuzzer
            PROPERTIES
            BUILD_RPATH "$ORIGIN"
            INSTALL_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../../../base-sdds/files/base/lib64/"
            )
    install(TARGETS CorcPolicyProcessorFuzzer
            DESTINATION files/plugins/${PLUGIN_NAME}/bin
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
            )
    SET_TARGET_PROPERTIES( CorePolicyProcessorFuzzer
            PROPERTIES
            BUILD_RPATH "$ORIGIN"
            INSTALL_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../../../base-sdds/files/base/lib64/"
            )
    install(TARGETS CorePolicyProcessorFuzzer
            DESTINATION files/plugins/${PLUGIN_NAME}/bin
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
            )
endif (DEFINED ENV{USE_LIBFUZZER})
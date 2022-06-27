*** Settings ***
Suite Setup      Suite Setup Without Ostia
Suite Teardown   Suite Teardown Without Ostia

Test Setup       Test Setup
Test Teardown    Run Keywords
...                Stop Local SDDS3 Server   AND
...                 Test Teardown

Test Timeout  10 mins

Library     ${LIBS_DIRECTORY}/WarehouseGenerator.py
Library     ${LIBS_DIRECTORY}/ThinInstallerUtils.py
Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library     ${LIBS_DIRECTORY}/MCSRouter.py
Library     ${LIBS_DIRECTORY}/UpdateSchedulerHelper.py
Library     ${LIBS_DIRECTORY}/TemporaryDirectoryManager.py
Library     ${LIBS_DIRECTORY}/OSUtils.py
Library     ${LIBS_DIRECTORY}/WarehouseUtils.py
Library     ${LIBS_DIRECTORY}/TeardownTools.py
Library     ${LIBS_DIRECTORY}/UpgradeUtils.py

Library     Process
Library     OperatingSystem
Library     String

Resource    ../event_journaler/EventJournalerResources.robot
Resource    ../mcs_router/McsRouterResources.robot
Resource    ../installer/InstallerResources.robot
Resource    ../thin_installer/ThinInstallerResources.robot
Resource    ../example_plugin/ExamplePluginResources.robot
Resource    ../av_plugin/AVResources.robot
Resource    ../mdr_plugin/MDRResources.robot
Resource    ../edr_plugin/EDRResources.robot
Resource    ../runtimedetections_plugin/RuntimeDetectionsResources.robot
Resource    ../scheduler_update/SchedulerUpdateResources.robot
Resource    ../GeneralTeardownResource.robot
Resource    UpgradeResources.robot
Resource    ../management_agent/ManagementAgentResources.robot

*** Variables ***
${BaseEdrAndMtrAndAVDogfoodPolicy}          ${GeneratedWarehousePolicies}/base_edr_and_mtr_and_av_VUT-1.xml
${BaseEdrAndMtrAndAVReleasePolicy}          ${GeneratedWarehousePolicies}/base_edr_and_mtr_and_av_Release.xml
${BaseAndMtrVUTPolicy}                      ${GeneratedWarehousePolicies}/base_and_mtr_VUT.xml
${BaseEdrAndMtrAndAVVUTPolicy}              ${GeneratedWarehousePolicies}/base_edr_and_mtr_and_av_VUT.xml
${BaseOnlyVUT_weekly_schedule_Policy}       ${GeneratedWarehousePolicies}/base_only_weeklyScheduleVUT.xml

${LocalWarehouseDir}                        ./local_warehouses
${SULDownloaderLog}                         ${SOPHOS_INSTALL}/logs/base/suldownloader.log
${SULDownloaderSyncLog}                         ${SOPHOS_INSTALL}/logs/base/suldownloader_sync.log
${SULDownloaderLogDowngrade}                ${SOPHOS_INSTALL}/logs/base/downgrade-backup/suldownloader.log
${UpdateSchedulerLog}                       ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log
${Sophos_Scheduled_Query_Pack}              ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf
${status_file}                              ${SOPHOS_INSTALL}/base/mcs/status/ALC_status.xml

${sdds3_override_file}                      ${SOPHOS_INSTALL}/base/update/var/sdds3_override_settings.ini


${sdds2_primary}                            ${SOPHOS_INSTALL}/base/update/cache/primary
${sdds2_primary_warehouse}                  ${SOPHOS_INSTALL}/base/update/cache/primarywarehouse

*** Test Cases ***
Sul Downloader fails update if expected product missing from SUS
    Start Local Cloud Server  --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_policy_FakePlugin.xml
    ${handle}=  Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}
    Require Fresh Install
    Create File    /opt/sophos-spl/base/mcs/certs/ca_env_override_flag
    Create Local SDDS3 Override

    Register With Local Cloud Server

    Wait Until Keyword Succeeds
    ...   20 secs
    ...   5 secs
    ...   File Should Contain  ${UPDATE_CONFIG}     ServerProtectionLinux-Plugin-Fake

    Trigger Update Now

    Wait Until Keyword Succeeds
    ...   20 secs
    ...   5 secs
    ...   Check Suldownloader Log Contains   Failed to connect to repository: Package : ServerProtectionLinux-Plugin-Fake missing from warehouse

Sul Downloader Can Update Via Sdds3 Repository And Removes Local SDDS2 Cache
    [Setup]    Test Setup With Ostia
    [Teardown]    Test Teardown With Ostia

    Start Local Cloud Server  --initial-alc-policy  ${BaseEdrAndMtrAndAVVUTPolicy}
    ${handle}=  Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}

    configure_and_run_SDDS3_thininstaller  0  http://127.0.0.1:8080   http://127.0.0.1:8080  ${True}

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check Log Contains String At Least N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Update success  2
    Check Local SDDS2 Cache Has Contents

    Create Local SDDS3 Override
    Trigger Update Now
    #TODO put this section back once LINUXDAR-4379 is done
#    Wait Until Keyword Succeeds
#    ...   10 secs
#    ...   2 secs
#    ...   Directory Should Exist   ${SOPHOS_INSTALL}/base/update/cache/sdds3primaryrepository
#    Wait Until Keyword Succeeds
#    ...   200 secs
#    ...   10 secs
#    ...   Directory Should Exist   ${SOPHOS_INSTALL}/base/update/cache/sdds3primaryrepository/suite
#    Wait Until Keyword Succeeds
#    ...   200 secs
#    ...   2 secs
#    ...   Directory Should Exist   ${SOPHOS_INSTALL}/base/update/cache/sdds3primaryrepository/package
#    Wait Until Keyword Succeeds
#    ...   200 secs
#    ...   2 secs
#    ...   Directory Should Exist   ${SOPHOS_INSTALL}/base/update/cache/sdds3primaryrepository/supplement
#    Wait Until Keyword Succeeds
#    ...   120 secs
#    ...   10 secs
#    ...   Directory Should Exist   ${SOPHOS_INSTALL}/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/
    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check Log Contains String At Least N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Update success  3
    Check Local SDDS2 Cache Is Empty
    Check Log Contains String N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Generating the report file  3

SDDS3 updating respects ALC feature codes
    [Setup]    Test Setup With Ostia
    [Teardown]    Test Teardown With Ostia

    Start Local Cloud Server  --initial-alc-policy  ${BaseEdrAndMtrAndAVVUTPolicy}
    ${handle}=  Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}

    configure_and_run_SDDS3_thininstaller  0  http://127.0.0.1:8080   http://127.0.0.1:8080  ${True}

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check Log Contains String At Least N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Update success  2
    Check Local SDDS2 Cache Has Contents

    Create Local SDDS3 Override
    Send Policy File  alc  ${SUPPORT_FILES}/CentralXml/ALC_CORE_only_feature_code.policy.xml  wait_for_policy=${True}
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...   120 secs
    ...   10 secs
    ...   Check Log Contains String At Least N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Update success  3
    Check Log Contains String N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Generating the report file  3
    #core plugins should be installed
    Directory Should Exist   ${SOPHOS_INSTALL}/plugins/eventjournaler
    Directory Should Exist   ${SOPHOS_INSTALL}/plugins/runtimedetections
    #other plugins should be uninstalled
    Directory Should Not Exist   ${SOPHOS_INSTALL}/plugins/av
    Directory Should Not Exist   ${SOPHOS_INSTALL}/plugins/edr
    Directory Should Not Exist   ${SOPHOS_INSTALL}/plugins/liveresponse
    Directory Should Not Exist   ${SOPHOS_INSTALL}/plugins/mtr
    Send Policy File  alc  ${SUPPORT_FILES}/CentralXml/FakeCloudDefaultPolicies/FakeCloudDefault_ALC_policy.xml  wait_for_policy=${True}
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...   120 secs
    ...   10 secs
    ...   Check Log Contains String At Least N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Update success  4
    Check Log Contains String N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Generating the report file  4
    Directory Should Exist   ${SOPHOS_INSTALL}/plugins/av
    Directory Should Exist   ${SOPHOS_INSTALL}/plugins/edr
    Directory Should Exist   ${SOPHOS_INSTALL}/plugins/liveresponse
    Directory Should Exist   ${SOPHOS_INSTALL}/plugins/mtr

SDDS3 updating with changed unused feature codes do not change version
    [Setup]    Test Setup With Ostia
    [Teardown]    Test Teardown With Ostia

    Start Local Cloud Server  --initial-alc-policy  ${BaseEdrAndMtrAndAVVUTPolicy}
    ${handle}=  Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}

    configure_and_run_SDDS3_thininstaller  0  http://127.0.0.1:8080   http://127.0.0.1:8080  ${True}

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check Log Contains String At Least N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Update success  2
    Check Local SDDS2 Cache Has Contents

    Create Local SDDS3 Override
    Send Policy File  alc  ${SUPPORT_FILES}/CentralXml/FakeCloudDefaultPolicies/FakeCloudDefault_ALC_policy.xml  wait_for_policy=${True}
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...   120 secs
    ...   10 secs
    ...   Check Log Contains String At Least N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Update success  3

    ${BaseVersionBeforeUpdate} =     Get Version Number From Ini File   ${InstalledBaseVersionFile}
    ${MtrVersionBeforeUpdate} =      Get Version Number From Ini File   ${InstalledMDRPluginVersionFile}
    ${EdrVersionBeforeUpdate} =      Get Version Number From Ini File   ${InstalledEDRPluginVersionFile}
    ${LrVersionBeforeUpdate} =      Get Version Number From Ini File   ${InstalledLRPluginVersionFile}
    ${AVVersionBeforeUpdate} =      Get Version Number From Ini File   ${InstalledAVPluginVersionFile}
    ${RuntimeDetectionsVersionBeforeUpdate} =      Get Version Number From Ini File   ${InstalledRuntimedetectionsPluginVersionFile}
    ${EJVersionBeforeUpdate} =      Get Version Number From Ini File    ${InstalledEJPluginVersionFile}
    ${HBTVersionBeforeUpdate} =      Get Version Number From Ini File    ${InstalledHBTPluginVersionFile}

    Send Policy File  alc  ${SUPPORT_FILES}/CentralXml/ALC_fake_feature_codes_policy.xml  wait_for_policy=${True}
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...   120 secs
    ...   10 secs
    ...   Check Log Contains String At Least N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Update success  3
    Check Log Contains String N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Generating the report file  3
    #TODO once defect LINUXDAR-4592 is done check here that no plugins are reinstalled as well
    ${BaseVersionAfterUpdate} =     Get Version Number From Ini File   ${InstalledBaseVersionFile}
    ${MtrVersionAfterUpdate} =      Get Version Number From Ini File   ${InstalledMDRPluginVersionFile}
    ${EdrVersionAfterUpdate} =      Get Version Number From Ini File   ${InstalledEDRPluginVersionFile}
    ${LrVersionAfterUpdate} =      Get Version Number From Ini File   ${InstalledLRPluginVersionFile}
    ${AVVersionAfterUpdate} =      Get Version Number From Ini File   ${InstalledAVPluginVersionFile}
    ${RuntimeDetectionsVersionAfterUpdate} =      Get Version Number From Ini File   ${InstalledRuntimedetectionsPluginVersionFile}
    ${EJVersionAfterUpdate} =      Get Version Number From Ini File    ${InstalledEJPluginVersionFile}
    ${HBTVersionAfterUpdate} =      Get Version Number From Ini File    ${InstalledHBTPluginVersionFile}
    Should Be Equal As Strings  ${RuntimeDetectionsVersionBeforeUpdate}  ${RuntimeDetectionsVersionAfterUpdate}
    Should Be Equal As Strings  ${MtrVersionBeforeUpdate}  ${MtrVersionAfterUpdate}
    Should Be Equal As Strings  ${EdrVersionBeforeUpdate}  ${EdrVersionAfterUpdate}
    Should Be Equal As Strings  ${LrVersionBeforeUpdate}  ${LrVersionAfterUpdate}
    Should Be Equal As Strings  ${AVVersionBeforeUpdate}  ${AVVersionAfterUpdate}
    Should Be Equal As Strings  ${EJVersionBeforeUpdate}  ${EJVersionAfterUpdate}
    Should Be Equal As Strings  ${HBTVersionBeforeUpdate}  ${HBTVersionAfterUpdate}
    Should Be Equal As Strings  ${BaseVersionBeforeUpdate}  ${BaseVersionAfterUpdate}

We can Install With SDDS3 Perform an SDDS2 Initial Update With SDDS3 Flag False Then Update Using SDDS3 When Flag Turns True
    [Setup]    Test Setup With Ostia
    [Teardown]    Test Teardown With Ostia

    Start Local Cloud Server  --initial-alc-policy  ${BaseEdrAndMtrAndAVVUTPolicy}
    ${handle}=  Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}

    configure_and_run_SDDS3_thininstaller  0  http://127.0.0.1:8080   http://127.0.0.1:8080  ${True}

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check Log Contains String At Least N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Update success  2

    #check first two updates are SDDS3 (install) followed by SDDS2 update (5th line only appears in sdds2 mode)
    Check Log Contains String N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Running in SDDS3 updating mode  1
    Check Log Contains In Order
    ...  ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    ...  Logger suldownloader configured for level: INFO
    ...  Running in SDDS3 updating mode
    ...  Update success
    ...  Logger suldownloader configured for level: INFO
    ...  Products downloaded and synchronized with warehouse
    ...  Update success

    #Don't force SDDS3 as we want to use flags to trigger it.
    Create Local SDDS3 Override  USE_SDDS3_OVERRIDE=false

    #prevent mcs overwriting flags
    File Should Contain  ${UPDATE_CONFIG}     "useSDDS3": false
    Overwrite MCS Flags File  {"sdds3.enabled": true}
    Wait Until Keyword Succeeds
    ...     10s
    ...     2s
    ...     File Should Contain  ${UPDATE_CONFIG}     "useSDDS3": true
    Mark Sul Log
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check Marked Sul Log Contains    Running in SDDS3 updating mode
    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check Log Contains String At Least N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Update success  3
    Check Log Contains String N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Generating the report file  3
    Check Log Contains String N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Running in SDDS3 updating mode  2

We can Install With SDDS3 Perform an SDDS3 Initial Update With SDDS3 Flag True Then Update Using SDDS2 When Flag Turns False
    [Setup]    Test Setup With Ostia
    [Teardown]    Test Teardown With Ostia And Fake Cloud MCS Flag Override
    ${desired_flags} =     Catenate    SEPARATOR=\n
    ...  {
    ...    "livequery.network-tables.available" : true,
    ...    "endpoint.flag2.enabled" : false,
    ...    "endpoint.flag3.enabled" : false,
    ...    "jwt-token.available" : true,
    ...    "mcs.v2.data_feed.available": true,
    ...    "sdds3.enabled": true
    ...  }

    Create File  /tmp/mcs_flags  ${desired_flags}
    Start Local Cloud Server  --initial-alc-policy  ${BaseEdrAndMtrAndAVVUTPolicy}
    ${handle}=  Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}

    configure_and_run_SDDS3_thininstaller  0  http://127.0.0.1:8080   http://127.0.0.1:8080  ${True}

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check Log Contains String At Least N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Update success  2

    #check first two updates are SDDS3 (installation + initial update)
    Check Log Contains String N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Running in SDDS3 updating mode  2
    Check Log Contains In Order
    ...  ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    ...  Logger suldownloader configured for level: INFO
    ...  Running in SDDS3 updating mode
    ...  Update success
    ...  Logger suldownloader configured for level: INFO
    ...  Running in SDDS3 updating mode
    ...  Update success

    #prevent mcs overwriting flags
    File Should Contain  ${UPDATE_CONFIG}     "useSDDS3": true
    Overwrite MCS Flags File  {"sdds3.enabled": false}
    Wait Until Keyword Succeeds
    ...     10s
    ...     2s
    ...     File Should Contain  ${UPDATE_CONFIG}     "useSDDS3": false
    Mark Sul Log
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check Marked Sul Log Does Not Contain    Running in SDDS3 updating mode
    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check Log Contains String At Least N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Update success  3
    Check Log Contains String N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Generating the report file  3
    Check Log Contains String N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Running in SDDS3 updating mode  2

*** Keywords ***
Create Dummy Local SDDS2 Cache Files
    Create File         ${sdds2_primary}/base/update/cache/primary/1
    Create Directory    ${sdds2_primary}/base/update/cache/primary/2
    Create File         ${sdds2_primary}/base/update/cache/primary/2f
    Create Directory    ${sdds2_primary}/base/update/cache/primary/2d
    Create File         ${sdds2_primary_warehouse}/base/update/cache/primarywarehouse/1
    Create Directory    ${sdds2_primary_warehouse}/base/update/cache/primarywarehouse/2
    Create File         ${sdds2_primary_warehouse}/base/update/cache/primarywarehouse/2f
    Create Directory    ${sdds2_primary_warehouse}/base/update/cache/primarywarehouse/2d

Directory Should Be Empty
    [Arguments]    ${directory_path}
    ${contents} =    List Directory    ${directory_path}
    Log    ${contents}
    Should Be Equal    ${contents.__len__()}    ${0}

Directory Should Not Be Empty
    [Arguments]    ${directory_path}
    ${contents} =    List Directory    ${directory_path}
    Log    ${contents}
    Should Not Be Equal    ${contents.__len__()}    ${0}

Check Local SDDS2 Cache Is Empty
    Directory Should Be Empty    ${sdds2_primary}
    Directory Should Be Empty    ${sdds2_primary_warehouse}

Check Local SDDS2 Cache Has Contents
    Directory Should Not Be Empty    ${sdds2_primary}
    Directory Should Not Be Empty    ${sdds2_primary_warehouse}

Test Setup With Ostia
    Test Setup
    Setup Ostia Warehouse Environment

Test Teardown With Ostia
    Stop Local SDDS3 Server
    Teardown Ostia Warehouse Environment
    Test Teardown

Test Teardown With Ostia And Fake Cloud MCS Flag Override
    Test Teardown With Ostia
    Remove File  /tmp/mcs_flags
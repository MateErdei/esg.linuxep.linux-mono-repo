*** Settings ***
Suite Setup      Suite Setup Without Ostia
Suite Teardown   Suite Teardown Without Ostia

Test Setup       Test Setup
Test Teardown    Run Keywords
...                Stop Local SDDS3 Server   AND
...                Test Teardown

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
Library     ${LIBS_DIRECTORY}/TelemetryUtils.py

Library     Process
Library     OperatingSystem
Library     String

Resource    ../mcs_router/McsRouterResources.robot
Resource    ../installer/InstallerResources.robot
Resource    ../scheduler_update/SchedulerUpdateResources.robot
Resource    UpgradeResources.robot
Resource    ../telemetry/TelemetryResources.robot

*** Variables ***
${BaseEdrAndMtrAndAVDogfoodPolicy}          ${GeneratedWarehousePolicies}/base_edr_and_mtr_and_av_VUT-1.xml
${BaseEdrAndMtrAndAVReleasePolicy}          ${GeneratedWarehousePolicies}/base_edr_and_mtr_and_av_Release.xml
${BaseAndMtrVUTPolicy}                      ${GeneratedWarehousePolicies}/base_and_mtr_VUT.xml
${BaseEdrAndMtrAndAVVUTPolicy}              ${GeneratedWarehousePolicies}/base_edr_and_mtr_and_av_VUT.xml
${BaseOnlyVUT_weekly_schedule_Policy}       ${GeneratedWarehousePolicies}/base_only_weeklyScheduleVUT.xml

${LocalWarehouseDir}                        ./local_warehouses
${SULDownloaderLog}                         ${SOPHOS_INSTALL}/logs/base/suldownloader.log
${SULDownloaderSyncLog}                     ${SOPHOS_INSTALL}/logs/base/suldownloader_sync.log
${SULDownloaderLogDowngrade}                ${SOPHOS_INSTALL}/logs/base/downgrade-backup/suldownloader.log
${UpdateSchedulerLog}                       ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log
${Sophos_Scheduled_Query_Pack}              ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf
${status_file}                              ${SOPHOS_INSTALL}/base/mcs/status/ALC_status.xml

${sdds3_override_file}                      ${SOPHOS_INSTALL}/base/update/var/sdds3_override_settings.ini


${sdds2_primary}                            ${SOPHOS_INSTALL}/base/update/cache/primary
${sdds2_primary_warehouse}                  ${SOPHOS_INSTALL}/base/update/cache/primarywarehouse
${sdds3_primary}                            ${SOPHOS_INSTALL}/base/update/cache/sdds3primary
${sdds3_primary_repository}                 ${SOPHOS_INSTALL}/base/update/cache/sdds3primaryrepository

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

    Configure And Run SDDS3 Thininstaller  0  https://localhost:8080   https://localhost:8080

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check Log Contains String At Least N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Update success  2
    Override LogConf File as Global Level  DEBUG
    Check Local SDDS2 Cache Has Contents

    Create Local SDDS3 Override
    Trigger Update Now

    Wait Until Keyword Succeeds
    ...   10 secs
    ...   2 secs
    ...   Directory Should Exist   ${sdds3_primary_repository}
    Wait Until Keyword Succeeds
    ...   200 secs
    ...   10 secs
    ...   Directory Should Exist   ${sdds3_primary_repository}/suite
    Wait Until Keyword Succeeds
    ...   200 secs
    ...   2 secs
    ...   Directory Should Exist   ${sdds3_primary_repository}/package
    Wait Until Keyword Succeeds
    ...   200 secs
    ...   2 secs
    ...   Directory Should Exist   ${sdds3_primary_repository}/supplement
    Wait Until Keyword Succeeds
    ...   240 secs
    ...   10 secs
    ...   Directory Should Exist   ${sdds3_primary}/ServerProtectionLinux-Base-component/

    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check Log Contains String At Least N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Update success  3
    Check Suldownloader Log Contains In Order    Update success    Purging local SDDS2 cache    Update success

    Check Local SDDS2 Cache Is Empty

    Check Log Contains String N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Generating the report file  3


SDDS3 updating respects ALC feature codes
    [Setup]    Test Setup With Ostia
    [Teardown]    Test Teardown With Ostia

    Start Local Cloud Server  --initial-alc-policy  ${BaseEdrAndMtrAndAVVUTPolicy}
    ${handle}=  Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}

    Configure And Run SDDS3 Thininstaller  0  https://localhost:8080   https://localhost:8080

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

    Configure And Run SDDS3 Thininstaller  0  https://localhost:8080   https://localhost:8080

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
    Should Be Equal As Strings  ${RuntimeDetectionsVersionBeforeUpdate}  ${RuntimeDetectionsVersionAfterUpdate}
    Should Be Equal As Strings  ${MtrVersionBeforeUpdate}  ${MtrVersionAfterUpdate}
    Should Be Equal As Strings  ${EdrVersionBeforeUpdate}  ${EdrVersionAfterUpdate}
    Should Be Equal As Strings  ${LrVersionBeforeUpdate}  ${LrVersionAfterUpdate}
    Should Be Equal As Strings  ${AVVersionBeforeUpdate}  ${AVVersionAfterUpdate}
    Should Be Equal As Strings  ${EJVersionBeforeUpdate}  ${EJVersionAfterUpdate}
    Should Be Equal As Strings  ${BaseVersionBeforeUpdate}  ${BaseVersionAfterUpdate}


We can Install With SDDS3 Perform an SDDS2 Initial Update With SDDS3 Flag False Then Update Using SDDS3 When Flag Turns True
    [Setup]    Test Setup With Ostia
    [Teardown]    Test Teardown With Ostia

    Start Local Cloud Server  --initial-alc-policy  ${BaseEdrAndMtrAndAVVUTPolicy}
    ${handle}=  Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}

    Configure And Run SDDS3 Thininstaller  0  https://localhost:8080   https://localhost:8080

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check Log Contains String At Least N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Update success  2

    #check first two updates are SDDS3 (install) followed by SDDS2 update (5th line only appears in sdds2 mode)
    Check Log Contains String N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Running in SDDS3 updating mode  1
    Check Suldownloader Log Contains In Order
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

    Configure And Run SDDS3 Thininstaller  0  https://localhost:8080   https://localhost:8080

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check Log Contains String At Least N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Update success  2

    #check first two updates are SDDS3 (installation + initial update)
    Check Log Contains String N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Running in SDDS3 updating mode  2
    Check Suldownloader Log Contains In Order
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


Consecutive SDDS3 Updates Without Changes Should Not Trigger Additional Installations of Components
    [Setup]    Test Setup With Ostia
    [Teardown]    Test Teardown With Ostia

    Start Local Cloud Server  --initial-alc-policy  ${BaseEdrAndMtrAndAVVUTPolicy}
    ${handle}=  Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}

    Configure And Run SDDS3 Thininstaller  0  https://localhost:8080   https://localhost:8080   force_sdds3_post_install=${True}

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check Log Contains String At Least N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Update success  2
    Check Log Contains String At Least N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Running in SDDS3 updating mode  2

    Mark Sul Log
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check Marked Sul Log Contains   Update success
    Check Marked Sul Log Does Not Contain  Installing product

    Check Marked Sul Log Contains   Downloaded Product line: 'ServerProtectionLinux-Base-component' is up to date.
    Check Marked Sul Log Contains   Downloaded Product line: 'ServerProtectionLinux-Plugin-MDR' is up to date.
    Check Marked Sul Log Contains   Downloaded Product line: 'ServerProtectionLinux-Plugin-EDR' is up to date.
    Check Marked Sul Log Contains   Downloaded Product line: 'ServerProtectionLinux-Plugin-AV' is up to date.
    Check Marked Sul Log Contains   Downloaded Product line: 'ServerProtectionLinux-Plugin-liveresponse' is up to date.
    Check Marked Sul Log Contains   Downloaded Product line: 'ServerProtectionLinux-Plugin-RuntimeDetections' is up to date.
    Check Marked Sul Log Contains   Downloaded Product line: 'ServerProtectionLinux-Plugin-EventJournaler' is up to date.
    ${latest_report} =  Run Shell Process  ls ${SOPHOS_INSTALL}/base/update/var/updatescheduler/update_report* -rt | cut -f 1 | tail -n1  shell=${True}
    All Products In Update Report Are Up To Date  ${latest_report.strip()}


During Transition From SDDS3 to SDDS2 SDDS3 Cache Is Removed Before Downloading SDDS2 Files
    [Setup]    Test Setup With Ostia
    [Teardown]    Test Teardown With Ostia

    Start Local Cloud Server  --initial-alc-policy  ${BaseEdrAndMtrAndAVVUTPolicy}
    ${handle}=  Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}

    Configure And Run SDDS3 Thininstaller  0  https://localhost:8080   https://localhost:8080

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check Log Contains    Update success    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log

    Override LogConf File as Global Level  DEBUG
    Wait Until Keyword Succeeds
    ...   60 secs
    ...   5 secs
    ...   Check Local SDDS3 Cache Has Contents

    Trigger Update Now

    Wait Until Keyword Succeeds
    ...  150 secs
    ...  10 secs
    ...  Check Suldownloader Log Contains In Order    Update success    Purging local SDDS3 cache    Update success

    Check Local SDDS3 Cache Is Empty
    Check Local SDDS2 Cache Has Contents

Schedule Query Pack Next Exists in SDDS3 and is Equal to Schedule Query Pack
    [Setup]    Test Setup With Ostia
    [Teardown]    Test Teardown With Ostia

    Start Local Cloud Server  --initial-alc-policy  ${BaseEdrAndMtrAndAVVUTPolicy}
    ${handle}=  Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}

    Configure And Run SDDS3 Thininstaller  0  https://localhost:8080   https://localhost:8080
    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check Log Contains String At Least N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Update success  2
    Create Local SDDS3 Override
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...   60 secs
    ...   5 secs
    ...   Check Local SDDS3 Cache Has Contents

    Wait Until Keyword Succeeds
    ...  300 secs
    ...  10 secs
    ...  Directory Should Exist  ${sdds3_primary}/ServerProtectionLinux-Plugin-EDR
    Wait Until Keyword Succeeds
    ...  120 secs
    ...  10 secs
    ...  Directory Should Exist  ${sdds3_primary}/ServerProtectionLinux-Plugin-EDR/scheduled_query_pack
    Wait Until Keyword Succeeds
    ...  120 secs
    ...  10 secs
    ...  Directory Should Exist  ${sdds3_primary}/ServerProtectionLinux-Plugin-EDR/scheduled_query_pack_next

    ${scheduled_query_pack} =             Get File    ${sdds3_primary}/ServerProtectionLinux-Plugin-EDR/scheduled_query_pack/sophos-scheduled-query-pack.conf
    ${scheduled_query_pack_next} =        Get File    ${sdds3_primary}/ServerProtectionLinux-Plugin-EDR/scheduled_query_pack_next/sophos-scheduled-query-pack.conf
    Should Be Equal As Strings            ${scheduled_query_pack}    ${scheduled_query_pack_next}

    ${scheduled_query_pack_mtr} =         Get File    ${sdds3_primary}/ServerProtectionLinux-Plugin-EDR/scheduled_query_pack/sophos-scheduled-query-pack.mtr.conf
    ${scheduled_query_pack_next_mtr} =    Get File    ${sdds3_primary}/ServerProtectionLinux-Plugin-EDR/scheduled_query_pack_next/sophos-scheduled-query-pack.mtr.conf
    Should Be Equal As Strings            ${scheduled_query_pack_mtr}    ${scheduled_query_pack_next_mtr}


SDDS3 Mechanism Is Updated in UpdateScheduler Telemetry After Successful Update To SDDS3
    [Setup]    Test Setup With Ostia
    [Teardown]    Test Teardown With Ostia
    Cleanup Telemetry Server
    Start Local Cloud Server  --initial-alc-policy  ${BaseEdrAndMtrAndAVVUTPolicy}
    ${handle}=  Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}
    Configure And Run SDDS3 Thininstaller  0  https://localhost:8080   https://localhost:8080

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check Log Contains String At Least N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Update success  2
    Set Log Level For Component And Reset and Return Previous Log  updatescheduler   DEBUG

    Wait Until Keyword Succeeds
    ...    30 secs
    ...    5 secs
    ...    Check UpdateScheduler Log Contains String N Times    Sending status to Central    2

    Prepare To Run Telemetry Executable With HTTPS Protocol    port=6443
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log File   ${TELEMETRY_OUTPUT_JSON}
    Check Update Scheduler Telemetry Json Is Correct  ${telemetryFileContents}  0   sddsid=av_user_vut  set_edr=True  set_av=True  most_recent_update_successful=True  sdds_mechanism=SDDS2

    Overwrite MCS Flags File    {"sdds3.enabled": true}
    Wait Until Keyword Succeeds
    ...     10s
    ...     2s
    ...     File Should Contain  ${UPDATE_CONFIG}     "useSDDS3": true
    Check Updatescheduler Log Contains    Received sdds3.enabled flag value: 1

    Trigger Update Now
    Wait Until Keyword Succeeds
    ...    200 secs
    ...    10 secs
    ...    Check Suldownloader Log Contains In Order    Update success    Running in SDDS3 updating mode   Generating the report file
    Wait Until Keyword Succeeds
    ...    30 secs
    ...    5 secs
    ...    Check UpdateScheduler Log Contains String N Times    Sending status to Central    3

    Cleanup Telemetry Server
    Prepare To Run Telemetry Executable With HTTPS Protocol    port=6443
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log File   ${TELEMETRY_OUTPUT_JSON}
    Check Update Scheduler Telemetry Json Is Correct  ${telemetryFileContents}  0   sddsid=av_user_vut  set_edr=True  set_av=True  most_recent_update_successful=True  sdds_mechanism=SDDS3
    Cleanup Telemetry Server

*** Keywords ***
Create Dummy Local SDDS2 Cache Files
    Create File         ${sdds2_primary}/1
    Create Directory    ${sdds2_primary}/2
    Create File         ${sdds2_primary}/2f
    Create Directory    ${sdds2_primary}/2d
    Create File         ${sdds2_primary_warehouse}/1
    Create Directory    ${sdds2_primary_warehouse}/2
    Create File         ${sdds2_primary_warehouse}/2f
    Create Directory    ${sdds2_primary_warehouse}/2d

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

Check Local SDDS3 Cache Is Empty
    Directory Should Be Empty    ${sdds3_primary}
    Directory Should Be Empty    ${sdds3_primary_repository}

Check Local SDDS2 Cache Has Contents
    Directory Should Not Be Empty    ${sdds2_primary}
    Directory Should Not Be Empty    ${sdds2_primary_warehouse}

Check Local SDDS3 Cache Has Contents
    Directory Should Not Be Empty    ${sdds3_primary}
    Directory Should Not Be Empty    ${sdds3_primary_repository}

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
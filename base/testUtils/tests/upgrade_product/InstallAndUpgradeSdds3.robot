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
${UpdateConfigFile}                         ${SOPHOS_INSTALL}/base/update/var/updatescheduler/update_config.json
${sdds3_server_output}                      /tmp/sdds3_server.log

${sdds2_primary}                            ${SOPHOS_INSTALL}/base/update/cache/primary
${sdds2_primary_warehouse}                  ${SOPHOS_INSTALL}/base/update/cache/primarywarehouse

*** Test Cases ***
Sul Downloader Can Update Via Sdds3 Repository
    Start Local Cloud Server
    ${handle}=  Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}
    Require Fresh Install
    Create File    /opt/sophos-spl/base/mcs/certs/ca_env_override_flag
    Create Local SDDS3 Override
    # should be purged before SDDS3 sync
    Create Dummy Local SDDS2 Cache Files
    Register With Local Cloud Server

    sleep    10
    Remove File  ${status_file}
    Remove FIle   /opt/sophos-spl/base/mcs/status/cache/ALC.xml

    File Should Contain  ${UpdateConfigFile}     JWToken
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...   10 secs
    ...   2 secs
    ...   Directory Should Exist   ${SOPHOS_INSTALL}/base/update/cache/sdds3primaryrepository
    Directory Should Exist   ${SOPHOS_INSTALL}/base/update/cache/sdds3primary

    Wait Until Keyword Succeeds
    ...   200 secs
    ...   10 secs
    ...   Directory Should Exist   ${SOPHOS_INSTALL}/base/update/cache/sdds3primaryrepository/suite
    Wait Until Keyword Succeeds
    ...   200 secs
    ...   2 secs
    ...   Directory Should Exist   ${SOPHOS_INSTALL}/base/update/cache/sdds3primaryrepository/package
    Wait Until Keyword Succeeds
    ...   200 secs
    ...   2 secs
    ...   Directory Should Exist   ${SOPHOS_INSTALL}/base/update/cache/sdds3primaryrepository/supplement

    Wait Until Keyword Succeeds
    ...   120 secs
    ...   10 secs
    ...   Directory Should Exist   ${SOPHOS_INSTALL}/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/

    Wait Until Keyword Succeeds
    ...   120 secs
    ...   10 secs
    ...   Check Suldownloader Log Contains   Update success
    Wait Until Keyword Succeeds
    ...   10 secs
    ...   1 secs
    ...   Check Suldownloader Log Contains   Generating the report file

    Check Local SDDS2 Cache Is Empty

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
    ...   File Should Contain  ${UpdateConfigFile}     ServerProtectionLinux-Plugin-Fake

    Trigger Update Now

    Wait Until Keyword Succeeds
    ...   20 secs
    ...   5 secs
    ...   Check Suldownloader Log Contains   Failed to connect to repository: Package : ServerProtectionLinux-Plugin-Fake missing from warehouse

SDDS3 Sync Removes Local SDDS2 Cache
    Start Local Cloud Server  --initial-alc-policy  ${BaseEdrAndMtrAndAVVUTPolicy}
    ${handle}=  Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}

    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseEdrAndMtrAndAVVUTPolicy}

    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  1
    Check Local SDDS2 Cache Has Contents

    Create Local SDDS3 Override
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...   10 secs
    ...   2 secs
    ...   Directory Should Exist   ${SOPHOS_INSTALL}/base/update/cache/sdds3primaryrepository

    Wait Until Keyword Succeeds
    ...   10 secs
    ...   1 secs
    ...   Check Suldownloader Log Contains   suldownloaderdata <> Performing Sync
    Check Local SDDS2 Cache Is Empty
    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check Log Contains String At Least N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Update success  2
    Check Log Contains String N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Generating the report file  2

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

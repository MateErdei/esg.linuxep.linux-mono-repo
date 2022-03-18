*** Settings ***
Suite Setup      Suite Setup
Suite Teardown   Suite Teardown

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
${UpdateConfigFile}                          ${SOPHOS_INSTALL}/base/update/var/updatescheduler/update_config.json

*** Test Cases ***
Sul Downloader Can Update Via Sdds3 Repository
    Start Local Cloud Server
    ${handle}=  Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}
    Require Fresh Install
    Create File    /opt/sophos-spl/base/mcs/certs/ca_env_override_flag
    Register With Local Cloud Server
    Create Local SDDS3 Override

    sleep    10
    Remove File  ${status_file}
    Remove FIle   /opt/sophos-spl/base/mcs/status/cache/ALC.xml
    Log File    ${UpdateConfigFile}
    ${content}=  Get File    ${UpdateConfigFile}
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
    ...   Check Suldownloader Log Contains   Generating the report file

    Log File  ${SULDownloaderLog}
    Log File  ${SULDownloaderSyncLog}



*** Keywords ***
Create Local SDDS3 Override
    ${override_file_contents} =  Catenate    SEPARATOR=\n
    # these settings will instruct SulDownloader to update using SDDS3 via a local test HTTP server.
    ...  URLS = http://127.0.0.1:8080
    ...  CDN_URL = http://127.0.0.1:8080
    ...  USE_SDDS3 = true
    ...  USE_HTTP = true
    Create File    ${sdds3_override_file}    content=${override_file_contents}
    fail

Start Local SDDS3 Server
    ${handle}=  Start Process  python3  ${LIBS_DIRECTORY}/SDDS3server.py    --launchdarkly    /tmp/system-product-test-inputs/sdds3/launchdarkly    --sdds3    /tmp/system-product-test-inputs/sdds3/repo   shell=true
    [Return]  ${handle}

Stop Local SDDS3 Server
     terminate process  ${GL_handle}  True
     terminate all processes  True


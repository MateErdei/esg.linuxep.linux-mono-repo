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
Resource    ../upgrade_product/UpgradeResources.robot
Resource    ../management_agent/ManagementAgentResources.robot


Default Tags  TAP_TESTS  SULDOWNLOADER


*** Variables ***
${fixed_version_policy}                     ${SUPPORT_FILES}/CentralXml/ALC_FixedVersionPolicy.xml

${status_file}                              ${SOPHOS_INSTALL}/base/mcs/status/ALC_status.xml

${sdds3_override_file}                      ${SOPHOS_INSTALL}/base/update/var/sdds3_override_settings.ini
${sdds3_server_output}                      /tmp/sdds3_server.log

*** Test Cases ***
Sul Downloader Requests Fixed Version When Fixed Version In Policy
    Start Local Cloud Server    --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_FixedVersionPolicy.xml
    ${handle}=  Start Local SDDS3 Server With Empty Repo
    Set Suite Variable    ${GL_handle}    ${handle}
    Require Fresh Install
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override
    # should be purged before SDDS3 sync
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...    5s
    ...    1s
    ...    Log File    ${UPDATE_CONFIG}
    ${content}=  Get File    ${UPDATE_CONFIG}
    File Should Contain  ${UPDATE_CONFIG}     JWToken
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...   10 secs
    ...   2 secs
    ...   File Should Contain    ${sdds3_server_output}     ServerProtectionLinux-Base fixedVersion: 1.1.0 requested
    Wait Until Keyword Succeeds
    ...   2 secs
    ...   1 secs
    ...   File Should Contain    ${sdds3_server_output}     ServerProtectionLinux-Plugin-MDR fixedVersion: 1.0.2 requested

SDDS2 Is Used When No useSDDS3 Field In Update Config
    Start Local Cloud Server
    Generate Warehouse From Local Base Input
    ${handle}=  Start Local SDDS3 server with fake files   port=8080
    Set Suite Variable    ${GL_handle}    ${handle}

    Setup Install SDDS3 Base
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    # should be purged before SDDS3 sync
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...    5s
    ...    1s
    ...    Log File    ${UPDATE_CONFIG}
    ${content}=  Get File    ${UPDATE_CONFIG}
    File Should Contain  ${UPDATE_CONFIG}     JWToken

    Trigger Update Now
    Wait Until Keyword Succeeds
    ...    5s
    ...    1s
    ...    Check Marked Sul Log Contains   Generating the report file

    remove_useSDDS3_from_update_config  ${UPDATE_CONFIG}
    Log File  ${UPDATE_CONFIG}
    Mark Sul Log
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...    5s
    ...    1s
    ...    Check Marked Sul Log Contains   Generating the report file
    Check Marked Sul Log Contains   Update failed, with code: 112
    Check Marked Sul Log Does Not Contain  Running in SDDS3 updating mode

set value of useSDDS3 to a non boolean in update config
    Start Local Cloud Server
    Generate Warehouse From Local Base Input
    ${handle}=  Start Local SDDS3 server with fake files   port=8080
    Set Suite Variable    ${GL_handle}    ${handle}

    Setup Install SDDS3 Base
    Create File    ${MCS_DIR}/certs/ca_env_override_flag

    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...    5s
    ...    1s
    ...    Log File    ${UPDATE_CONFIG}
    ${content}=  Get File    ${UPDATE_CONFIG}
    File Should Contain  ${UPDATE_CONFIG}     JWToken

    Trigger Update Now
    Wait Until Keyword Succeeds
    ...    10s
    ...    1s
    ...    Check Marked Sul Log Contains   Generating the report file

    set_useSDDS3_in_update_config_to_value  ${UPDATE_CONFIG}  ${1}
    sleep  5
    Log File  ${UPDATE_CONFIG}
    Mark Sul Log
    Run Shell Process  systemctl start sophos-spl-update     failed to start suldownloader
    Wait Until Keyword Succeeds
    ...    10s
    ...    1s
    ...    Check Marked Sul Log Contains   Generating the report file
    Check Marked Sul Log Contains   Update failed, with code: 121
    Check Marked Sul Log Contains  Failed to process json message with error: INVALID_ARGUMENT:(useSDDS3): invalid value 1 for type TYPE_BOOL



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

File Should Contain
    [Arguments]  ${file_path}  ${expected_contents}
    ${contents}=  Get File   ${file_path}
    Should Contain  ${contents}   ${expected_contents}

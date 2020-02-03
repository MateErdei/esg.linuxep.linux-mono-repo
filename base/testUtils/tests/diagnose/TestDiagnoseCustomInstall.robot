*** Settings ***

Library     ${libs_directory}/LogUtils.py
Library     ${libs_directory}/DiagnoseUtils.py
Library     ${libs_directory}/OSUtils.py

Library     Process
Library     OperatingSystem

Resource    ../management_agent-audit_plugin/AuditPluginResources.robot
Resource    ../management_agent-event_processor/EventProcessorResources.robot
Resource    DiagnoseResources.robot

Suite Setup     Suite Setup Custom Install
Suite Teardown  Suite Teardown Custom Install

Test Setup      Should Exist  ${SOPHOS_INSTALL}/bin/sophos_diagnose
Test Teardown   Teardown

Default Tags  DIAGNOSE

*** Variables ***

${CustomPath}  /opt/not-default-location


*** Test Cases ***
Diagnose Tool Gathers Logs When Run From Installation In Custom Location
    [Tags]  EVENT_PLUGIN  AUDIT_PLUGIN  DIAGNOSE  CUSTOM_LOCATION
    Install Audit Plugin Directly
    Install EventProcessor Plugin Directly

    Wait Until Created  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcs_envelope.log     20 seconds

    Create Directory  ${TAR_FILE_DIRECTORY}
    ${retcode} =  Run Diagnose    ${SOPHOS_INSTALL}/bin/     ${TAR_FILE_DIRECTORY}
    LogUtils.Dump Log    /tmp/diagnose.log
    Should Be Equal As Integers   ${retcode}  0

    # Check diagnose tar created
    ${Files} =  List Files In Directory  ${TAR_FILE_DIRECTORY}/
    ${fileCount} =    Get length    ${Files}
    Should Be Equal As Numbers  ${fileCount}  1
    Should Contain    ${Files[0]}    sspl-diagnose
    Should Not Contain   ${Files}  BaseFiles
    Should Not Contain   ${Files}  SystemFiles
    Should Not Contain   ${Files}  PluginFiles

    # Untar diagnose tar to check contents
    Create Directory  ${UNPACK_DIRECTORY}
    ${result} =   Run Process   tar    xzf    ${TAR_FILE_DIRECTORY}/${Files[0]}    -C    ${UNPACK_DIRECTORY}/
    Should Be Equal As Strings   ${result.rc}  0

    Check Diagnose Base Output
    Check Diagnose Output For Plugin logs
    Check Diagnose Output For System Command Files
    Check Diagnose Output For System Files

    ${contents} =  Get File  /tmp/diagnose.log
    Should Not Contain  ${contents}  error  ignore_case=True
    Should Contain  ${contents}   Created tarfile: ${Files[0]} in directory ${TAR_FILE_DIRECTORY}


*** Keywords ***
Suite Setup Custom Install
    Set Sophos Install Environment Variable  ${CustomPath}/sophos-spl
    Set Suite Variable  ${SOPHOS_INSTALL_CACHE}  ${SOPHOS_INSTALL}
    Set Global Variable  ${SOPHOS_INSTALL}  ${CustomPath}/sophos-spl
    Require Fresh Install

Suite Teardown Custom Install
    Uninstall SSPL  ${SOPHOS_INSTALL}
    Set Global Variable  ${SOPHOS_INSTALL}  ${SOPHOS_INSTALL_CACHE}
    Reset Sophos Install Environment Variable
    Remove Dir If Exists  ${CustomPath}

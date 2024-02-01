*** Settings ***

Library     Process
Library     OperatingSystem
Library     Collections

Library     ${COMMON_TEST_LIBS}/DiagnoseUtils.py
Library     ${COMMON_TEST_LIBS}/OSUtils.py
Library     ${COMMON_TEST_LIBS}/LogUtils.py

Resource    ${COMMON_TEST_ROBOT}/DiagnoseResources.robot
Resource    ${COMMON_TEST_ROBOT}/EDRResources.robot
Resource    ${COMMON_TEST_ROBOT}/LiveResponseResources.robot
Resource    ${COMMON_TEST_ROBOT}/ResponseActionsResources.robot
Resource    ${COMMON_TEST_ROBOT}/RuntimeDetectionsResources.robot

Suite Setup  Require Fresh Install
Suite Teardown  Ensure Uninstalled

Test Setup      Diagnose Test Setup
Test Teardown   Diagnose Test Teardown

Force Tags  TAP_PARALLEL2    DIAGNOSE

*** Test Cases ***
Diagnose Tool Gathers LR Logs When Run From Installation
    [Tags]   LIVE_RESPONSE
    Wait Until Created  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcs_envelope.log     20 seconds

    Create Directory  ${TAR_FILE_DIRECTORY}

    Install Live Response Directly
    Mimic LR Component Files   ${SOPHOS_INSTALL}

    ${retcode} =  Run Diagnose    ${SOPHOS_INSTALL}/bin/     ${TAR_FILE_DIRECTORY}
    Should Be Equal As Integers   ${retcode}  0

    # Check diagnose tar created
    ${Files} =  List Files In Directory  ${TAR_FILE_DIRECTORY}/
    ${fileCount} =    Get length    ${Files}
    Should Be Equal As Numbers  ${fileCount}  1
    Should Contain    ${Files[0]}    sspl-diagnose
    Should Not Contain   ${Files}  BaseFiles
    Should Not Contain   ${Files}  SystemFiles
    Should Not Contain   ${Files}  PluginFiles


    ${folder}=  Fetch From Left   ${Files[0]}   .tar.gz
    Set Suite Variable  ${DiagnoseOutput}  ${folder}
    # Untar diagnose tar to check contents
    Create Directory  ${UNPACK_DIRECTORY}
    ${result} =   Run Process   tar    xzf    ${TAR_FILE_DIRECTORY}/${Files[0]}    -C    ${UNPACK_DIRECTORY}/
    Should Be Equal As Strings   ${result.rc}  0

    Check Diagnose Output For Additional LR Plugin Files
    Check Diagnose Output For System Command Files
    Check Diagnose Output For System Files

    ${contents} =  Get File  /tmp/diagnose.log
    Should Not Contain  ${contents}  error  ignore_case=True
    Should Contain  ${contents}   Created tarfile: ${Files[0]} in directory ${TAR_FILE_DIRECTORY}

Diagnose Tool Gathers RuntimeDetections Logs When Run From Installation
    [Tags]  RUNTIMEDETECTIONS_PLUGIN
    Wait Until Created  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcs_envelope.log     20 seconds

    Create Directory  ${TAR_FILE_DIRECTORY}

    Install RuntimeDetections Directly

    Wait Until Keyword Succeeds
        ...   10 secs
        ...   1 secs
        ...   File Should Exist  ${SOPHOS_INSTALL}/plugins/runtimedetections/log/runtimedetections.log

    ${retcode} =  Run Diagnose    ${SOPHOS_INSTALL}/bin/     ${TAR_FILE_DIRECTORY}
    Should Be Equal As Integers   ${retcode}  0

    # Check diagnose tar created
    ${Files} =  List Files In Directory  ${TAR_FILE_DIRECTORY}/
    ${fileCount} =    Get length    ${Files}
    Should Be Equal As Numbers  ${fileCount}  1
    ${folder}=  Fetch From Left   ${Files[0]}   .tar.gz
    Set Suite Variable  ${DiagnoseOutput}  ${folder}
    # Untar diagnose tar to check contents
    Create Directory  ${UNPACK_DIRECTORY}
    ${result} =   Run Process   tar    xzf    ${TAR_FILE_DIRECTORY}/${Files[0]}    -C    ${UNPACK_DIRECTORY}/
    Should Be Equal As Strings   ${result.rc}  0

    Check Diagnose Output For Additional RuntimeDetections Plugin Files
    Check Diagnose Output For System Command Files
    Check Diagnose Output For System Files

    ${contents} =  Get File  /tmp/diagnose.log
    Should Not Contain  ${contents}  error  ignore_case=True
    Should Contain  ${contents}   Created tarfile: ${Files[0]} in directory ${TAR_FILE_DIRECTORY}


Diagnose Tool Gathers EDR Logs When Run From Installation
    #WARNING this test should be the last in the suite to avoid watchdog going down due to the defect LINUXDAR-3732
    [Tags]    EDR_PLUGIN
    Wait Until Created  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcs_envelope.log     20 seconds

    Create Directory  ${TAR_FILE_DIRECTORY}

    Install EDR Directly
    Restart EDR Plugin    clearLog=True    installQueryPacks=True
    Wait Until Keyword Succeeds
        ...   10 secs
        ...   1 secs
        ...   File Should Exist  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf

    Copy File  ${SUPPORT_FILES}/xdr-query-packs/error-queries.conf  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf
    Copy File  ${SUPPORT_FILES}/xdr-query-packs/error-queries.conf  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.mtr.conf
    Wait Until Keyword Succeeds
        ...   10 secs
        ...   1 secs
        ...   File Should Exist  ${SOPHOS_INSTALL}/plugins/edr/log/edr.log
    Wait Until Keyword Succeeds
        ...   10 secs
        ...   1 secs
        ...   File Should Exist  ${SOPHOS_INSTALL}/plugins/edr/log/osqueryd.results.log
    ${retcode} =  Run Diagnose    ${SOPHOS_INSTALL}/bin/     ${TAR_FILE_DIRECTORY}
    Should Be Equal As Integers   ${retcode}  0

    # Check diagnose tar created
    ${Files} =  List Files In Directory  ${TAR_FILE_DIRECTORY}/
    ${fileCount} =    Get length    ${Files}
    Should Be Equal As Numbers  ${fileCount}  1

    ${folder}=  Fetch From Left   ${Files[0]}   .tar.gz
    Set Suite Variable  ${DiagnoseOutput}  ${folder}
    # Untar diagnose tar to check contents
    Create Directory  ${UNPACK_DIRECTORY}
    ${result} =   Run Process   tar    xzf    ${TAR_FILE_DIRECTORY}/${Files[0]}    -C    ${UNPACK_DIRECTORY}/
    Should Be Equal As Strings   ${result.rc}  0

    log file  /tmp/diagnose.log
    Check Diagnose Output For Additional EDR Plugin File
    Check Diagnose Output For System Command Files
    Check Diagnose Output For System Files

    ${contents} =  Get File  /tmp/diagnose.log
    Should Not Contain  ${contents}  error  ignore_case=True
    Should Contain  ${contents}   Created tarfile: ${Files[0]} in directory ${TAR_FILE_DIRECTORY}

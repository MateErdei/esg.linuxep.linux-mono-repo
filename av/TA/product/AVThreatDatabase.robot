*** Settings ***
Documentation    Product tests of SafeStore
Force Tags       INTEGRATION  SAFESTORE

Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVAndBaseResources.robot
Resource    ../shared/AVResources.robot
Resource    ../shared/ErrorMarkers.robot

Library         ../Libs/CoreDumps.py
Library         ../Libs/OnFail.py
Library         ../Libs/ProcessUtils.py

Library         OperatingSystem
Library         Collections

Test Setup      ThreatDatabase Test Setup
Test Teardown   ThreatDatabase Test TearDown
*** Variables ***
${THREAT_DATABASE_PATH}        ${SOPHOS_INSTALL}/plugins/av/var/persist-threatDatabase

*** Test Cases ***
Threat is added to Threat database when threat is not quarantined
    Start AV
    ${avmark} =  get_av_log_mark
    Create File     ${NORMAL_DIRECTORY}/naughty_eicar    ${EICAR_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/naughty_eicar
    wait_for_av_log_contains_after_mark   Added threat: T26796de6ce94770 to database   ${avmark}
    Stop AV
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  File Log Contains  ${THREAT_DATABASE_PATH}  T26796de6ce94770
    ${avmark} =  get_av_log_mark
    ${handle} =  Start Process  ${AV_PLUGIN_BIN}
    Set Suite Variable  ${AV_PLUGIN_HANDLE}  ${handle}
    wait_for_av_log_contains_after_mark  Initialised Threat Database   ${avmark}
    Remove File  ${THREAT_DATABASE_PATH}
    Stop AV  #file should be written
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  File Log Contains  ${THREAT_DATABASE_PATH}  T26796de6ce94770

Threat is removed from Threat database when marked as resolved in central
    Start AV
    ${avmark} =  get_av_log_mark
    Create File     ${NORMAL_DIRECTORY}/naughty_eicar    ${EICAR_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/naughty_eicar
    wait_for_av_log_contains_after_mark   Added threat: T26796de6ce94770 to database   ${avmark}
    ${actionContent} =  Set Variable  <action type="sophos.mgt.action.SAVClearFromList" ><threat-set><threat id="T26796de6ce94770"></threat></threat-set></action>
    Send Plugin Action  av  sav  corr123  ${actionContent}
    wait_for_av_log_contains_after_mark   Removed threat from database   ${avmark}
    wait_for_av_log_contains_after_mark   Publishing threat health: good   ${avmark}

Threat is not added to Threat database when threat is quarantined
    Start AV
    Start SafeStore Manually
    ${avmark} =  get_av_log_mark
    ${policyContent}=    Get File   ${RESOURCES_PATH}/flags_policy/flags_safestore_enabled.json
    Send Plugin Policy  av  FLAGS  ${policyContent}
    wait_for_av_log_contains_after_mark     SafeStore flag set. Setting SafeStore to enabled   ${avmark}  timeout=60
    Create File     ${NORMAL_DIRECTORY}/naughty_eicar    ${EICAR_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/naughty_eicar
    wait_for_av_log_contains_after_mark  Quarantine succeeded  ${avmark}
    Stop AV
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  File Log Contains  ${THREAT_DATABASE_PATH}  {}

*** Keywords ***
ThreatDatabase Test Setup
    Component Test Setup
    register on fail  dump log  ${THREAT_DETECTOR_LOG_PATH}
    register on fail  dump log  ${WATCHDOG_LOG}
    register on fail  dump log  ${SAFESTORE_LOG_PATH}
    register on fail  dump log  ${SOPHOS_INSTALL}/logs/base/wdctl.log
    register on fail  dump log  ${SOPHOS_INSTALL}/plugins/av/log/av.log
    register on fail  dump log   ${SUSI_DEBUG_LOG_PATH}
    register on fail  dump threads  ${SOPHOS_THREAT_DETECTOR_BINARY}
    register on fail  dump threads  ${PLUGIN_BINARY}

    Register Cleanup      Check All Product Logs Do Not Contain Error
    Register Cleanup      Exclude MCS Router is dead
    Register Cleanup      Exclude CustomerID Failed To Read Error
    Register Cleanup      Require No Unhandled Exception
    Register Cleanup      Check For Coredumps  ${TEST NAME}
    Register Cleanup      Check Dmesg For Segfaults

Stop AV
    ${proc} =  Get Process Object  ${AV_PLUGIN_HANDLE}
    Log  Stopping AV Plugin Process PID=${proc.pid}
    ${result} =  Terminate Process  ${AV_PLUGIN_HANDLE}
    Log  ${result.stderr}
    Log  ${result.stdout}
    Remove Files   /tmp/av.stdout  /tmp/av.stderr

ThreatDatabase Test TearDown
    Stop AV
    Stop SafeStore Manually
    Remove File  ${THREAT_DATABASE_PATH}
    run teardown functions
    Component Test TearDown

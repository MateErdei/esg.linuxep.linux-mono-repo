*** Settings ***
Documentation    Product tests of SafeStore
Force Tags       PRODUCT  SAFESTORE

Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVAndBaseResources.robot
Resource    ../shared/AVResources.robot
Resource    ../shared/ErrorMarkers.robot
Resource    ../shared/SafeStoreResources.robot

Library         ../Libs/CoreDumps.py
Library         ../Libs/FakeManagementLog.py
Library         ../Libs/OnFail.py
Library         ../Libs/ProcessUtils.py

Library         OperatingSystem
Library         Collections

Test Setup      ThreatDatabase Test Setup
Test Teardown   ThreatDatabase Test TearDown

*** Test Cases ***
Threat is added to Threat database when threat is not quarantined
    ${fake_management_log_path} =   FakeManagementLog.get_fake_management_log_path
    Register On Fail  Dump Log  ${fake_management_log_path}
    Start AV
    ${avmark} =  get_av_log_mark
    Create File     ${NORMAL_DIRECTORY}/naughty_eicar    ${EICAR_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/naughty_eicar
    wait_for_av_log_contains_after_mark   Added threat 265a4b8a-239b-5f7e-8e4b-c78748cbd7ef with correlationID   ${avmark}
    Stop AV
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  File Log Contains  ${THREAT_DATABASE_PATH}  265a4b8a-239b-5f7e-8e4b-c78748cbd7ef
    ${avmark} =  get_av_log_mark
    ${handle} =  Start Process  ${AV_PLUGIN_BIN}
    Set Suite Variable  ${AV_PLUGIN_HANDLE}  ${handle}
    wait_for_av_log_contains_after_mark   Publishing threat health: suspicious   ${avmark}
    wait_for_av_log_contains_after_mark  Initialised Threat Database   ${avmark}
    Remove File  ${THREAT_DATABASE_PATH}
    Stop AV  #file should be written
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  File Log Contains  ${THREAT_DATABASE_PATH}  265a4b8a-239b-5f7e-8e4b-c78748cbd7ef

Threat is removed from Threat database when marked as resolved in central
    Start AV
    ${avmark} =  get_av_log_mark
    Create File     ${NORMAL_DIRECTORY}/naughty_eicar    ${EICAR_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/naughty_eicar
    wait_for_av_log_contains_after_mark   Added threat 265a4b8a-239b-5f7e-8e4b-c78748cbd7ef with correlationID   ${avmark}

    ## Only interested in removals after we send the action
    ${avmark} =  get_av_log_mark
    ${actionContent} =  Set Variable  <action type="sophos.core.threat.sav.clear"><item id="265a4b8a-239b-5f7e-8e4b-c78748cbd7ef"/></action>
    Send Plugin Action  av  ${SAV_APPID}  corr123  ${actionContent}
    wait_for_av_log_contains_after_mark   Removed threat from database   ${avmark}
    wait_for_av_log_contains_after_mark   Threat database is now empty, sending good health to Management agent   ${avmark}
    wait_for_av_log_contains_after_mark   Publishing threat health: good   ${avmark}

Threat database is cleared when we get a core reset action from central
    Start AV
    ${avmark} =  get_av_log_mark
    Create File     ${NORMAL_DIRECTORY}/naughty_eicar    ${EICAR_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/naughty_eicar
    wait_for_av_log_contains_after_mark   Added threat 265a4b8a-239b-5f7e-8e4b-c78748cbd7ef with correlationID   ${avmark}
    ${actionContent} =  Set Variable  <action type="sophos.core.threat.reset"/>
    Send Plugin Action  av  core  corr123  ${actionContent}
    wait_for_av_log_contains_after_mark   Resetting threat database due to core reset action   ${avmark}
    wait_for_av_log_contains_after_mark   Publishing threat health: good   ${avmark}

Threat is removed from Threat database when threat is quarantined
    ${avmark} =  get_av_log_mark
    Start AV
    # Start AV also starts Safestore
    Wait Until SafeStore running
    wait_for_av_log_contains_after_mark   Publishing threat health: good   ${avmark}
    ${avmark} =  get_av_log_mark
    Create File     ${NORMAL_DIRECTORY}/naughty_eicar    ${EICAR_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/naughty_eicar
    wait_for_av_log_contains_after_mark   Added threat 265a4b8a-239b-5f7e-8e4b-c78748cbd7ef with correlationID   ${avmark}
    ${avmark} =  get_av_log_mark
    ${policyContent}=    Get File   ${RESOURCES_PATH}/flags_policy/flags_safestore_enabled.json
    Send Plugin Policy  av  FLAGS  ${policyContent}
    wait_for_av_log_contains_after_mark     SafeStore flag set. Setting SafeStore to enabled   ${avmark}  timeout=60
    Create File     ${NORMAL_DIRECTORY}/naughty_eicar    ${EICAR_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/naughty_eicar
    wait_for_av_log_contains_after_mark  Quarantine succeeded  ${avmark}
    wait_for_av_log_contains_after_mark   Removed threat id 265a4b8a-239b-5f7e-8e4b-c78748cbd7ef from database   ${avmark}
    wait_for_av_log_contains_after_mark   Threat database is now empty, sending good health to Management agent   ${avmark}
    wait_for_av_log_contains_after_mark   Publishing threat health: good   ${avmark}
    Stop AV
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  File Log Contains  ${THREAT_DATABASE_PATH}  {}


Threat is not added to Threat database when threat is quarantined
    Start AV
    # Start AV also starts Safestore
    Wait Until SafeStore running

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
    run teardown functions
    Remove File  ${THREAT_DATABASE_PATH}
    Component Test TearDown

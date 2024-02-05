*** Settings ***
Documentation    Product tests of SafeStore
Force Tags       PRODUCT  SAFESTORE  TAP_PARALLEL1

Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVAndBaseResources.robot
Resource    ../shared/AVResources.robot
Resource    ../shared/ErrorMarkers.robot
Resource    ../shared/SafeStoreResources.robot

Library         ../Libs/FileSampleObfuscator.py
Library         ${COMMON_TEST_LIBS}/CoreDumps.py
Library         ../Libs/FakeManagementLog.py
Library         ${COMMON_TEST_LIBS}/OnFail.py
Library         ${COMMON_TEST_LIBS}/ProcessUtils.py

Library         OperatingSystem
Library         Collections

Test Setup      ThreatDatabase Test Setup
Test Teardown   ThreatDatabase Test TearDown

*** Test Cases ***
Threat is added to Threat database when threat is not quarantined
    ${fake_management_log_path} =   FakeManagementLog.get_fake_management_log_path
    Register On Fail  Dump Log  ${fake_management_log_path}
    Start AV
    ${avmark} =  Mark AV Log
    Create File     ${NORMAL_DIRECTORY}/naughty_eicar    ${EICAR_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/naughty_eicar
    wait_for_av_log_contains_after_mark   Added threat 265a4b8a-239b-5f7e-8e4b-c78748cbd7ef with correlationId   ${avmark}
    Stop AV
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  File Log Contains  ${THREAT_DATABASE_PATH}  265a4b8a-239b-5f7e-8e4b-c78748cbd7ef
    ${avmark} =  Mark AV Log
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
    ${avmark} =  Mark AV Log
    Create File     ${NORMAL_DIRECTORY}/naughty_eicar    ${EICAR_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/naughty_eicar
    ${threat_id} =  Set Variable  265a4b8a-239b-5f7e-8e4b-c78748cbd7ef
    ${correlation_id} =  Get Correlation Id From Log  ${avmark}  ${threat_id}

    ## Only interested in removals after we send the action
    ${avmark} =  Mark AV Log
    ${actionContent} =  Set Variable  <action type="sophos.core.threat.sav.clear"><item id="${correlation_id}"/></action>
    Send Plugin Action  av  ${SAV_APPID}  corr123  ${actionContent}
    wait_for_av_log_contains_after_mark   Removing threat ${threat_id} with correlationId ${correlation_id}  ${avmark}
    wait_for_av_log_contains_after_mark   Threat database is now empty, sending good health to Management agent   ${avmark}
    wait_for_av_log_contains_after_mark   Publishing threat health: good   ${avmark}

Threat database is cleared when we get a core reset action from central
    Start AV
    ${avmark} =  Mark AV Log
    Create File     ${NORMAL_DIRECTORY}/naughty_eicar    ${EICAR_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/naughty_eicar
    wait_for_av_log_contains_after_mark   Added threat 265a4b8a-239b-5f7e-8e4b-c78748cbd7ef with correlationId   ${avmark}
    ${actionContent} =  Set Variable  <action type="sophos.core.threat.reset"/>
    Send Plugin Action  av  core  corr123  ${actionContent}
    wait_for_av_log_contains_after_mark   Resetting threat database due to core reset action   ${avmark}
    wait_for_av_log_contains_after_mark   Publishing threat health: good   ${avmark}

Threat is removed from Threat database when threat is quarantined
    ${avmark} =  Mark AV Log
    Start AV
    # Start AV also starts Safestore
    Wait Until SafeStore running
    wait_for_av_log_contains_after_mark   Publishing threat health: good   ${avmark}
    ${avmark} =  Mark AV Log
    Create File     ${NORMAL_DIRECTORY}/naughty_eicar    ${EICAR_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/naughty_eicar
    wait_for_av_log_contains_after_mark   Added threat 265a4b8a-239b-5f7e-8e4b-c78748cbd7ef with correlationId   ${avmark}
    ${avmark} =  Mark AV Log
    Remove File    ${SOPHOS_INSTALL}/plugins/av/var/disable_safestore
    Create File     ${NORMAL_DIRECTORY}/naughty_eicar    ${EICAR_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/naughty_eicar
    wait_for_av_log_contains_after_mark   Threat cleaned up at path: '${NORMAL_DIRECTORY}/naughty_eicar'  ${avmark}
    wait_for_av_log_contains_after_mark   Removed threatId 265a4b8a-239b-5f7e-8e4b-c78748cbd7ef from database   ${avmark}
    wait_for_av_log_contains_after_mark   Threat database is now empty, sending good health to Management agent   ${avmark}
    wait_for_av_log_contains_after_mark   Publishing threat health: good   ${avmark}
    Stop AV
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  File Log Contains  ${THREAT_DATABASE_PATH}  {}


Threat is not added to Threat database when threat is quarantined
    set_default_policy_from_file  CORC   ${RESOURCES_PATH}/corc_policy/corc_policy_empty_allowlist.xml
    set_default_policy_from_file  CORE   ${RESOURCES_PATH}/core_policy/CORE-36_oa_disabled.xml
    set_default_policy_from_file  FLAGS  ${RESOURCES_PATH}/flags_policy/flags.json
    set_default_policy_from_file  SAV    ${RESOURCES_PATH}/sav_policy/SAV_default_policy.xml
    set_default_policy_from_file  ALC    ${RESOURCES_PATH}/alc_policy/template/base_and_av_VUT.xml
    Remove File    ${SOPHOS_INSTALL}/plugins/av/var/disable_safestore
    ${avmark} =  Mark AV Log
    Start AV
    # Start AV also starts Safestore
    Wait Until SafeStore running

    Create File     ${NORMAL_DIRECTORY}/threat_not_added_eicar    ${EICAR_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/threat_not_added_eicar
    wait_for_av_log_contains_after_mark  Threat cleaned up at path: '${NORMAL_DIRECTORY}/threat_not_added_eicar'  ${avmark}  timeout=60
    Stop AV
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  File Log Contains  ${THREAT_DATABASE_PATH}  {}


Duplicate Threat Event Is Not Sent To Central If Not Quarantined
    Start AV
    # Start AV also starts Safestore
    Wait Until SafeStore running

    ${avmark} =  Mark AV Log

    DeObfuscate File  ${RESOURCES_PATH}/file_samples_obfuscated/MLengHighScore.exe  ${NORMAL_DIRECTORY}/MLengHighScore-excluded.exe
    Register Cleanup  Remove File  ${NORMAL_DIRECTORY}/MLengHighScore-excluded.exe

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/MLengHighScore-excluded.exe
    wait_for_av_log_contains_after_mark   Found 'ML/PE-A' in '/home/vagrant/this/is/a/directory/for/scanning/MLengHighScore-excluded.exe' which is a new detection   ${avmark}
    wait_for_av_log_contains_after_mark   Threat database is not empty, sending suspicious health to Management agent   ${avmark}

    ${avmark} =  Mark AV Log
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/MLengHighScore-excluded.exe
    wait_for_av_log_contains_after_mark   Found 'ML/PE-A' in '/home/vagrant/this/is/a/directory/for/scanning/MLengHighScore-excluded.exe' which is a duplicate detection   ${avmark}

*** Keywords ***
ThreatDatabase Test Setup
    Component Test Setup
    register on fail dump logs
    register on fail  dump threads  ${SOPHOS_THREAT_DETECTOR_BINARY}
    register on fail  dump threads  ${PLUGIN_BINARY}
    register on fail  List AV Plugin Path

    Register Cleanup      Check All Product Logs Do Not Contain Error
    Register Cleanup      Exclude MCS Router is dead
    Register Cleanup      Exclude CustomerID Failed To Read Error
    Register Cleanup      Require No Unhandled Exception
    Create File    ${SOPHOS_INSTALL}/plugins/av/var/disable_safestore

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

Get Correlation Id From Log
    [Arguments]  ${mark}  ${threat_id}
    wait_for_av_log_contains_after_mark   Added threat ${threat_id} with correlationId   ${mark}
    ${marked_av_log} =  get_av_log_after_mark_as_unicode  ${mark}
    ${matches} =  Get Regexp Matches  ${marked_av_log}  Added threat ${threat_id} with correlationId (.*) to threat database  1
    IF  len($matches) != 1
        Fail  Wrong amount of matches
    END
    [Return]  ${matches}[0]
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
Suite Setup  ThreatDatabase Suite Setup
Test Setup      ThreatDatabase Test Setup
Test Teardown   ThreatDatabase Test TearDown
*** Variables ***
${THREAT_DATABASE_PATH}        ${SOPHOS_INSTALL}/plugins/av/var/persist-threatDatabase
*** Test Cases ***
Threat is added to Threat database when threat is not quarantined
    ${av_mark} =  get_av_log_mark
    Check avscanner can detect eicar
    wait_for_log_contains_from_mark  ${av_mark}  Added threat: Tbd7be297ddf3cd8 to database
    Stop AV Plugin
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  File Log Contains  ${THREAT_DATABASE_PATH}  Tbd7be297ddf3cd8
    ${av_mark} =  get_av_log_mark
    Start AV Plugin
    wait_for_log_contains_from_mark  ${av_mark}  Initialised Threat Database
    Remove File  ${THREAT_DATABASE_PATH}
    Stop AV Plugin  #file should be written
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  File Log Contains  ${THREAT_DATABASE_PATH}  Tbd7be297ddf3cd8

*** Keywords ***
ThreatDatabase Suite Setup
    Require Plugin Installed and Running

    ${customerIdFile} =   Set Variable   ${AV_PLUGIN_PATH}/var/customer_id.txt
    Create File  ${customerIdFile}  c1cfcf69a42311a6084bcefe8af02c8a


ThreatDatabase Test Setup
    Start AV Plugin Process

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

    # Safestore isn't running so don't expect it to communicate
    Register Cleanup      Exclude Safestore connection errors
    Register Cleanup      Require No Unhandled Exception
    Register Cleanup      Check For Coredumps  ${TEST NAME}
    Register Cleanup      Check Dmesg For Segfaults

ThreatDatabase Test TearDown
    run cleanup functions
    run failure functions if failed

    Stop AV Plugin
    Remove File  ${THREAT_DATABASE_PATH}

    run keyword if test failed  Restart AV Plugin And Clear The Logs For Integration Tests

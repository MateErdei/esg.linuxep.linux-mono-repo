*** Settings ***
Documentation    Tests for the SSPLAV Plugin

Resource  BaseResources.robot

Library     ../libs/Cleanup.py
Library     ../libs/LogUtils.py

Force Tags  SSPLAV
Suite Setup  SDDS3 server setup
Suite Teardown  SDDS3 Server Teardown
Test Setup      Base Test Setup
Test Teardown   Teardown

*** Test Cases ***
Thin Installer can install SSPLAV
    [Tags]       FAKE_CENTRAL
    [Timeout]    10 minutes
    Start Fake Cloud
    Create Thin Installer  https://localhost:4443/mcs
    Run Thin Installer  /tmp/SophosInstallCombined.sh  ${0}  http://localhost:1233  ${SUPPORT_FILES}/https/ca/root-ca.crt.pem
    Verify That MCS Connection To Central Is Established

    register on fail  dump log  ${SULDOWNLOADER_LOG_PATH}
    register on fail  dump log  ${UPDATESCHEDULER_LOG_PATH}
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  10 sec
    ...  check_log_contains_string_n_times   ${SULDOWNLOADER_LOG_PATH}    Update success   2
    Send ALC Policy With AV

    Wait For ALC Policy On Endpoint
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 sec
    ...  Update Config Should Contain AV
    Run Update Now

    Wait For SSPLAV to be installed

*** Variables ***
${POLICY} =  ${SUPPORT_FILES}/CentralXml/RealWarehousePolicies/GeneratedAlcPolicies/base_and_av_VUT.xml
${BASE_LOGS}                        ${SOPHOS_INSTALL}/logs/base
${SULDOWNLOADER_LOG_PATH}           ${BASE_LOGS}/suldownloader.log
${UPDATESCHEDULER_LOG_PATH}         ${BASE_LOGS}/sophosspl/updatescheduler.log
${MCS_DIR} =                        ${SOPHOS_INSTALL}/base/mcs
${ALC_POLICY_PATH} =                ${MCS_DIR}/policy/ALC-1_policy.xml
${UPDATE_CONFIG_PATH} =             ${SOPHOS_INSTALL}/base/update/var/updatescheduler/update_config.json

*** Keywords ***

Verify That MCS Connection To Central Is Established
    [Arguments]  ${Port}=4443
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 sec
    ...  Check Mcsrouter Log Contains    Successfully directly connected to localhost:${Port}


Send ALC Policy With AV
    Send Policy File  alc  ${POLICY}


Wait For ALC Policy On Endpoint
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 sec
    ...  Check ALC Policy On Endpoint  ${POLICY}

Update Config Should Contain AV
    ${contents} =  get file  ${UPDATE_CONFIG_PATH}
    Should contain   ${contents}   ServerProtectionLinux-Plugin-AV

Check ALC Policy On Endpoint
    [Arguments]  ${EXPECTED_POLICY_FILE}=${POLICY}
    ${EXPECTED} =  get file  ${EXPECTED_POLICY_FILE}
    ${ACTUAL} =  get file  ${ALC_POLICY_PATH}
    Should Be Equal  ${ACTUAL}  ${EXPECTED}

Run Update Now
    trigger update now

Wait For SSPLAV to be installed
    Wait Until Keyword Succeeds
    ...  10 min
    ...  10 secs
    ...  Check if SSPLAV is installed

Check if SSPLAV is installed
    Directory Should Exist  ${SSPLAV_DIR}

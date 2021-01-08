*** Settings ***
Documentation    Tests for the SSPLAV Plugin

Resource  BaseResources.robot

Library     ../libs/Cleanup.py
Library     ../libs/LogUtils.py

Force Tags  WAREHOUSE  SSPLAV

Test Setup      Base Test Setup
Test Teardown   Teardown

*** Test Cases ***
Thin Installer can install SSPLAV
    [Tags]  FAKE_CENTRAL
    Start Fake Cloud
    Setup Warehouses
    Create Thin Installer  https://localhost:4443/mcs
    Run Thin Installer  /tmp/SophosInstallCombined.sh  ${0}  http://localhost:1233  ${SUPPORT_FILES}/https/ca/root-ca.crt.pem
    Verify That MCS Connection To Central Is Established

    register on fail  dump log  ${SULDOWNLOADER_LOG_PATH}
    register on fail  dump log  ${UPDATESCHEDULER_LOG_PATH}

    Send ALC Policy With AV

    Wait For ALC Policy On Endpoint

    Run Update Now

    Wait For SSPLAV to be installed

*** Variables ***
${POLICY} =  ${SUPPORT_FILES}/CentralXml/RealWarehousePolicies/GeneratedAlcPolicies/base_and_av_VUT.xml
${BASE_LOGS}                        ${SOPHOS_INSTALL}/logs/base
${SULDOWNLOADER_LOG_PATH}           ${BASE_LOGS}/suldownloader.log
${UPDATESCHEDULER_LOG_PATH}         ${BASE_LOGS}/sophosspl/updatescheduler.log
${MCS_DIR} =                        ${SOPHOS_INSTALL}/base/mcs
${ALC_POLICY_PATH} =                ${MCS_DIR}/policy/ALC-1_policy.xml

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
    ...  10 secs
    ...  1 sec
    ...  Check ALC Policy On Endpoint  ${POLICY}


Check ALC Policy On Endpoint
    [Arguments]  ${EXPECTED_POLICY_FILE}=${POLICY}
    ${EXPECTED} =  get file  ${EXPECTED_POLICY_FILE}
    ${ACTUAL} =  get file  ${ALC_POLICY_PATH}
    Should Be Equal  ${ACTUAL}  ${EXPECTED}

Run Update Now
    trigger update now

Wait For SSPLAV to be installed
    Wait Until Keyword Succeeds
    ...  2 min
    ...  10 secs
    ...  Check if SSPLAV is installed

Check if SSPLAV is installed
    Directory Should Exist  ${SSPLAV_DIR}

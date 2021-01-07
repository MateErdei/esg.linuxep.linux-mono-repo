*** Settings ***
Documentation    Tests for the SSPLAV Plugin

Resource  BaseResources.robot

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

    Send ALC Policy With AV

    Run Update Now

    Check SSPLAV installed

*** Keywords ***

Verify That MCS Connection To Central Is Established
    [Arguments]  ${Port}=4443
    Wait Until Keyword Succeeds
    ...  1 min
    ...  15 secs
    ...  Check Mcsrouter Log Contains    Successfully directly connected to localhost:${Port}


Send ALC Policy With AV
    Send Policy File  alc  ${SUPPORT_FILES}/CentralXml/RealWarehousePolicies/GeneratedAlcPolicies/base_and_av_VUT.xml

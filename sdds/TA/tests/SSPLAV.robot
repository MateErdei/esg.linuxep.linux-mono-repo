*** Settings ***
Documentation    Tests for the SSPLAV Plugin

Resource  BaseResources.robot

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

    Send ALC Policy With AV

    Run Update Now

    Check SSPLAV installed


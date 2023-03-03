*** Settings ***
Documentation   Test using thin installer to install base

Resource  BaseResources.robot

Force Tags  BASE
Suite Setup  SDDS3 server setup
Suite Teardown  SDDS3 Server Teardown
Test Setup      Base Test Setup
Test Teardown   Teardown


*** Test Case ***
Thin Installer can install Base to Dev Region
    [Tags]  DEV_CENTRAL  MANUAL
    Create Thin Installer
    Run Thin Installer  /tmp/SophosInstallCombined.sh  ${0}  http://localhost:1233  ${SUPPORT_FILES}/certs/hmr-dev-sha1.pem

    Check Base Installed

Thin Installer can install Base to Local Fake Cloud
    [Tags]  FAKE_CENTRAL
    Start Fake Cloud
    Create Thin Installer  https://localhost:4443/mcs
    Run Thin Installer  /tmp/SophosInstallCombined.sh  ${0}  http://localhost:1233  ${SUPPORT_FILES}/https/ca/root-ca.crt.pem

    Check Base Installed

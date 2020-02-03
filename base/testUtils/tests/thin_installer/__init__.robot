*** Settings ***
Library    Process
Library    ${libs_directory}/UpdateServer.py
Library    ${libs_directory}/WarehouseGenerator.py
Library    ${libs_directory}/ThinInstallerUtils.py
Library    ${libs_directory}/OSUtils.py
Library    ${libs_directory}/LogUtils.py
Resource   ./ThinInstallerResources.robot
Resource  ../GeneralTeardownResource.robot

Suite Setup      Setup Update Tests
Suite Teardown   Cleanup Update Tests

Test Teardown    Thin installer test teardown

*** Keywords ***

### Setup
Setup Update Tests
    Regenerate HTTPS Certificates
    Copy File   ${SUPPORT_FILES}/https/ca/root-ca.crt.pem    ${SUPPORT_FILES}/https/ca/root-ca.crt
    Install System Ca Cert  ${SUPPORT_FILES}/https/ca/root-ca.crt
    Uninstall SAV

### Cleanup
Cleanup Update Tests
    Cleanup System Ca Certs
    Run Process    make    clean    cwd=${SUPPORT_FILES}/https/

Thin installer test teardown
    General Test Teardown
    Stop Update Server
    Stop Proxy Servers
    Run Keyword If Test Failed    Dump Thininstaller Log
    Run Keyword If Test Failed    Dump Warehouse Log


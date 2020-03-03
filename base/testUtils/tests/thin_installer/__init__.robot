*** Settings ***
Library    Process
Library    ${LIBS_DIRECTORY}/UpdateServer.py
Library    ${LIBS_DIRECTORY}/WarehouseGenerator.py
Library    ${LIBS_DIRECTORY}/ThinInstallerUtils.py
Library    ${LIBS_DIRECTORY}/OSUtils.py
Library    ${LIBS_DIRECTORY}/LogUtils.py
Resource   ./ThinInstallerResources.robot
Resource  ../GeneralTeardownResource.robot
Resource  ../mcs_router/McsRouterResources.robot

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
    Regenerate Certificates
    Set Local CA Environment Variable

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


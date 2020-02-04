*** Settings ***

Suite Setup      Setup Update Tests
Suite Teardown   Cleanup Update Tests

Test Teardown    Run Keywords
...              General Test Teardown  AND
...              Stop Update Server    AND
...              Stop Proxy Servers     AND
...              Run Keyword If Test Failed    Dump Thininstaller Log  AND
...              Run Keyword If Test Failed    Dump Warehouse Log

Library    Process
Library    ${LIBS_DIRECTORY}/UpdateServer.py
Library    ${LIBS_DIRECTORY}/WarehouseGenerator.py
Library    ${LIBS_DIRECTORY}/ThinInstallerUtils.py
Library    ${LIBS_DIRECTORY}/LogUtils.py

Resource  ../GeneralTeardownResource.robot

*** Keywords ***

### Setup
Setup Update Tests
    Regenerate Certificates
    Uninstall SAV

Regenerate Certificates
    Run Process    make    clean    cwd=${SUPPORT_FILES}/https/
    Run Process    make    all    cwd=${SUPPORT_FILES}/https/

### Cleanup
Cleanup Update Tests
    Run Process    make    clean    cwd=${SUPPORT_FILES}/https/

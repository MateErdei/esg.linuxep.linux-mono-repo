*** Settings ***
Test Teardown    Run Keywords
...              General Test Teardown  AND
...              Stop Update Server    AND
...              Stop Proxy Servers     AND
...              Run Keyword If Test Failed    Dump Thininstaller Log  AND
...              Run Keyword If Test Failed    Dump Warehouse Log

Library    Process
Library    ${COMMON_TEST_LIBS}/UpdateServer.py
Library    ${COMMON_TEST_LIBS}/ThinInstallerUtils.py
Library    ${COMMON_TEST_LIBS}/LogUtils.py

Resource    ${COMMON_TEST_ROBOT}/GeneralTeardownResource.robot


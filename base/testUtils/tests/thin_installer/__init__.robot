*** Settings ***
Library    Process
Library    ${LIBS_DIRECTORY}/UpdateServer.py
Library    ${LIBS_DIRECTORY}/ThinInstallerUtils.py
Library    ${LIBS_DIRECTORY}/OSUtils.py
Library    ${COMMON_TEST_LIBS}/LogUtils.py

Resource    ${COMMON_TEST_ROBOT}/GeneralTeardownResource.robot
Resource    ${COMMON_TEST_ROBOT}/McsRouterResources.robot
Resource    ${COMMON_TEST_ROBOT}/SDDS3Resources.robot
Resource    ${COMMON_TEST_ROBOT}/ThinInstallerResources.robot
Resource    ${COMMON_TEST_ROBOT}/UpgradeResources.robot

Suite Setup      Setup base package
Suite Teardown   Cleanup base package
Test Teardown    Thin installer test teardown

# TODO LINUXDAR-4130: Enable for ARM64
Force Tags    TAP_PARALLEL4    EXCLUDE_ARM64

*** Keywords ***
Setup base package
    ${base_package}=    Generate Local Base SDDS3 Package
    Set Global Variable    ${sdds3_base_package}    ${base_package}

Cleanup base package
    Clean up local Base SDDS3 Package

Thin installer test teardown
    General Test Teardown
    Stop Update Server
    Stop Proxy Servers
    Run Keyword If Test Failed    Dump Thininstaller Log
    Run Keyword If Test Failed    Dump Warehouse Log


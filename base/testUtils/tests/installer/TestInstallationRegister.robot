*** Settings ***
Documentation    Test registering with Central with full installer

Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library    ${COMMON_TEST_LIBS}/LogUtils.py

Resource  ../installer/InstallerResources.robot
Resource  ../GeneralTeardownResource.robot

*** Test Cases ***
Register with Central with invalid arguments
    [Tags]    INSTALLER   MCS
    Ensure Uninstalled

    Run Full Installer Expecting Code  30  --mcs-url  "https://localhost:9134"  --mcs-token   "This is a token"


*** Keywords ***
Ensure Uninstalled
    Uninstall SSPL

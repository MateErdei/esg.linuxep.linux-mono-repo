*** Settings ***
Documentation    Test registering with Central with full installer

Library    ${libs_directory}/FullInstallerUtils.py
Library    ${libs_directory}/LogUtils.py

Resource  ../installer/InstallerResources.robot
Resource  ../GeneralTeardownResource.robot

*** Test Cases ***
Register with Central with invalid arguments
    [Tags]    INSTALLER   MCS
    Ensure Uninstalled

    Run Full Installer Expecting Code  30  --mcs-url  "https://localhost:9134"  --mcs-token   "This is a token"

Register with pulsar
    [Tags]    INSTALLER   MCS   CENTRAL  MANUAL

    ${MCS_URL} =  Set Variable  https://mcs.sandbox.sophos/sophos/management/ep
    ${MCS_TOKEN} =  Set Variable  a62013e61f869bb6a494e4acbca76b8979323cb727d9e0d803b7f98bbe8410ee

    Set Pulsar CA Environment Variable

    Ensure Uninstalled

    Run Full Installer  --mcs-url  ${MCS_URL}  --mcs-token   ${MCS_TOKEN}

*** Keywords ***
Ensure Uninstalled
    Uninstall SSPL

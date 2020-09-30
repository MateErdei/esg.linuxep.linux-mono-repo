*** Settings ***
Suite Setup      EDR Suite Setup
Suite Teardown   EDR Suite Teardown

Test Setup       EDR Test Setup
Test Teardown    EDR Test Teardown

Library     ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/MCSRouter.py

Resource    ../GeneralTeardownResource.robot
Resource    EDRResources.robot
Resource    ../mdr_plugin/MDRResources.robot

Default Tags   EDR_PLUGIN  FAKE_CLOUD
*** Test Cases ***
Flags Are Only Sent To EDR and Not MTR
    Install MDR Directly
    Register With Fake Cloud
    Wait Until Keyword Succeeds
    ...  30
    ...  1
    ...  Check MCS Router Running
    Wait Until OSQuery Running

    Wait Until Keyword Succeeds
        ...  10
        ...  1
        ...  Check EDR Log Contains  Applying new policy with APPID: FLAGS
    Check MTR Log Does Not Contain  Applying new policy with APPID: FLAGS
    Check Managementagent Log Contains  Policy flags.json applied to 1 plugins

*** Keywords ***
EDR Test Setup
    Install EDR Directly
    Start Local Cloud Server   --initial-alc-policy    ${GeneratedWarehousePolicies}/base_and_edr_VUT.xml

EDR Test Teardown
    Stop Local Cloud Server
    Run Keyword if Test Failed    Report Audit Link Ownership
    Run Keyword If Test Failed    Dump Cloud Server Log
    General Test Teardown
    Uninstall EDR Plugin

EDR Suite Setup
    Regenerate Certificates
    Require Fresh Install
    Set Local CA Environment Variable
    Override LogConf File as Global Level  DEBUG
    Check For Existing MCSRouter
    Cleanup MCSRouter Directories
    Cleanup Local Cloud Server Logs

EDR Suite Teardown
    Uninstall SSPL
    Unset CA Environment Variable

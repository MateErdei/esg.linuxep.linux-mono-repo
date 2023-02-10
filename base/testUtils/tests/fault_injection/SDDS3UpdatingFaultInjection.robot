*** Settings ***
Suite Setup      Suite Setup Without Ostia
Suite Teardown   Suite Teardown Without Ostia

Test Setup       Local Test Setup
Test Teardown    Local Test Teardown

Library     ${LIBS_DIRECTORY}/FaultInjectionTools.py
Library     ${LIBS_DIRECTORY}/ThinInstallerUtils.py

Resource    ../scheduler_update/SchedulerUpdateResources.robot
Resource    ../upgrade_product/UpgradeResources.robot

Default Tags    SULDOWNLOADER    FAULTINJECTION

*** Test Cases ***
SulDownloader Updates Via SDDS2 When useSDDS3 is Missing In Update Config
    Start Local Cloud Server  --initial-alc-policy  ${GeneratedWarehousePolicies}/base_edr_and_mtr_and_av_VUT.xml  --initial-flags  ${SUPPORT_FILES}/CentralXml/FLAGS_sdds2.json
    ${handle}=  Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}

    Configure And Run SDDS3 Thininstaller  0  https://localhost:8080   https://localhost:8080
    Wait Until Keyword Succeeds
    ...    200 secs
    ...    10 secs
    ...    Check SulDownloader Log Contains String N Times   Update success  2
    Mark Sul Log

    Remove Key From Json File    useSDDS3    ${UPDATE_CONFIG}
    File Should Not Contain  ${UPDATE_CONFIG}     useSDDS3

    Trigger Update Now
    Wait Until Keyword Succeeds
    ...    60 secs
    ...    5 secs
    ...    Check Marked Sul Log Contains    Running in SDDS2 updating mode

    Wait Until Keyword Succeeds
    ...    60 secs
    ...    5 secs
    ...    Check Marked Sul Log Contains    Update success

SulDownloader Fails Update When SDDS3 Enabled Flag Is Not A Bool
    [Timeout]  10 minutes
    Start Local Cloud Server    --initial-alc-policy    ${GeneratedWarehousePolicies}/base_edr_and_mtr_and_av_VUT.xml  --initial-flags  ${SUPPORT_FILES}/CentralXml/FLAGS_sdds2.json
    ${handle}=  Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}

    Configure And Run SDDS3 Thininstaller    0    https://localhost:8080   https://localhost:8080
    Override Local LogConf File for a component    DEBUG    global
    Wait Until Keyword Succeeds
    ...    200 secs
    ...    10 secs
    ...    Check SulDownloader Log Contains String N Times   Update success  2
    Mark Sul Log
    Mark UpdateScheduler Log

    Send Mock Flags Policy  {"sdds3.enabled": "true"}
    Wait Until Keyword Succeeds
    ...    30s
    ...    5s
    ...    Check Marked Update Scheduler Log Contains    sdds3.enabled flag value still: 0
    Check Update Config Contains Expected useSDDS3 Value    false

    Trigger Update Now
    Wait Until Keyword Succeeds
    ...    60 secs
    ...    5 secs
    ...    Check Marked Sul Log Contains    Running in SDDS2 updating mode
    Wait Until Keyword Succeeds
    ...    60 secs
    ...    5 secs
    ...    Check Marked Sul Log Contains    Update success
    Mark Sul Log

    Modify Value In Json File    useSDDS3    "true"    ${UPDATE_CONFIG}
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...    60 secs
    ...    5 secs
    ...    Check Marked Sul Log Contains    Failed to process json message with error: INVALID_ARGUMENT:(useSDDS3): invalid value ""true"" for type TYPE_BOOL
    Wait Until Keyword Succeeds
    ...    60 secs
    ...    5 secs
    ...    Check Marked Sul Log Contains    Update failed, with code: 121

*** Keywords ***
Local Test Setup
    Test Setup
    Setup Ostia Warehouse Environment

Local Test Teardown
    Stop Local SDDS3 Server
    Teardown Ostia Warehouse Environment
    Test Teardown

*** Settings ***
Documentation    Prove end-to-end management from Central

Library         ../Libs/CloudClient/CloudClient.py
Library         OperatingSystem
Library         String

Resource        ../shared/AVResources.robot

Suite Setup     No Operation
Suite Teardown  No Operation

Test Setup      No Operation
Test Teardown   No Operation

*** Keywords ***

Install Base And Plugin Without Register
    Install With Base SDDS
    ## MCS router stopped

Create Eicar
    [Arguments]  ${path}
    Create File      ${path}    ${EICAR_STRING}

*** Test Cases ***

Scan now from Central and Verify Scan Completed and Eicar Detected
    Select Central Region  DEV
    get central version
    Ensure AV Policy Exists
    Install Base And Plugin Without Register
    Register In Central
    Configure Exclude everything else in Central  /tmp/testeicar/
    Create Eicar  /tmp/testeicar/eicar.com
    Wait For exclusion configuration on endpoint
    send scan now in central
    Wait For Scan Now to start
    Wait For Scan Now to complete
    Wait For Scan Completion in central
    Wait For Eicar Detection in central

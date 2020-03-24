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

Check Specific File Content
    [Arguments]     ${expectedContent}  ${filePath}
    ${FileContents} =  Get File  ${filePath}
    Should Contain    ${FileContents}   ${expectedContent}

Wait For File With Particular Contents
    [Arguments]     ${expectedContent}  ${filePath}
    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Specific File Content  ${expectedContent}  ${filePath}
    Log File  ${filePath}

Wait For exclusion configuration on endpoint
    Wait For File With Particular Contents  /boot/  ${SOPHOS_INSTALL}/base/mcs/policy/SAV-2_policy.xml

*** Test Cases ***

Scan now from Central and Verify Scan Completed and Eicar Detected
    Select Central Region  DEV
    get central version
    Ensure AV Policy Exists
    Install Base And Plugin Without Register
    Register In Central
    Wait for computer to appear in Central
    Assign AntiVirus Product to Endpoint in Central
    Configure Exclude everything else in Central  /tmp/testeicar/
    Create Eicar  /tmp/testeicar/eicar.com
    Wait For exclusion configuration on endpoint
    send scan now in central
    Wait For Scan Now to start
    Wait For Scan Now to complete
    Wait For Scan Completion in central
    Wait For Eicar Detection in central

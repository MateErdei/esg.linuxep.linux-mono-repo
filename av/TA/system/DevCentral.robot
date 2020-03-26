*** Settings ***
Documentation    Prove end-to-end management from Central

Library         ../Libs/CloudClient/CloudClient.py
Library         DateTime
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

Wait for SAV policy on endpoint
    Wait Until Created  ${SOPHOS_INSTALL}/base/mcs/policy/SAV-2_policy.xml  5 mins

Check Specific File Content
    [Arguments]     ${expectedContent}  ${filePath}
    ${FileContents} =  Get File  ${filePath}
    Should Contain    ${FileContents}   ${expectedContent}

Wait For File With Particular Contents
    [Arguments]     ${expectedContent}  ${filePath}
    Wait Until Keyword Succeeds
    ...  5 min
    ...  15 secs
    ...  Check Specific File Content  ${expectedContent}  ${filePath}
    Log File  ${filePath}

Wait For exclusion configuration on endpoint
    Wait For File With Particular Contents  /boot/  ${SOPHOS_INSTALL}/base/mcs/policy/SAV-2_policy.xml
    Wait Until AV Plugin Log Contains  Updating scheduled scan configuration

Wait For Scan Time configuration on endpoint
    [Arguments]     ${scan_time}
    Wait For File With Particular Contents   <time>${scan_time}:00</time>  ${SOPHOS_INSTALL}/base/mcs/policy/SAV-2_policy.xml
    Wait Until AV Plugin Log Contains  Updating scheduled scan configuration

Wait For Scan Now to start
    Wait Until AV Plugin Log Contains  Starting Scan Now scan   120
    Wait Until AV Plugin Log Contains  Starting scan Scan Now

Wait For Central Scheduled Scan to start
    Wait Until AV Plugin Log Contains  Starting scan Sophos Cloud Scheduled Scan     1800

Wait For Scan Now to complete
    Wait Until AV Plugin Log Contains  Completed scan Scan Now

Wait For Central Scheduled Scan to complete
    Wait Until AV Plugin Log Contains  Completed scan Sophos Cloud Scheduled Scan


*** Test Cases ***

Scan now from Central and Verify Scan Completed and Eicar Detected
    [Tags]  SYSTEM  CENTRAL
    Select Central Region
    log central events
    clear alerts in central
    Ensure AV Policy Exists
    Install Base And Plugin Without Register
    Register In Central
    Wait for computer to appear in Central
    Assign AntiVirus Product to Endpoint in Central
    Configure Exclude everything else in Central  /tmp/testeicar/
    Create Eicar  /tmp/testeicar/eicar.com
    Wait For exclusion configuration on endpoint
    ${currentTime} =  Get Current Date  UTC  result_format=epoch
    send scan now in central
    Wait For Scan Now to start
    Wait For Scan Now to complete
    Wait For Scan Completion in central  ${currentTime}
    Wait For Eicar Detection in central  /tmp/testeicar/eicar.com  ${currentTime}

Scheduled Scan from Central and Verify Scan Completed and Eicar Detected
    [Tags]  SYSTEM  CENTRAL  MANUAL
    [Timeout]    40min
    Select Central Region
    log central events
    clear alerts in central
    Ensure AV Policy Exists
    Install Base And Plugin Without Register
    # Base will uninstall SSPL-AV if it does an update
    Remove File  ${SOPHOS_INSTALL}/base/update/certs/ps_rootca.crt
    Register In Central
    Wait for computer to appear in Central
    Assign AntiVirus Product to Endpoint in Central
    Create Eicar  /tmp/testeicar/eicar.com

    # Includes Configure Exclude everything else in Central  /tmp/testeicar/
    ${scan_time} =  Configure next available scheduled Scan in Central  /tmp/testeicar/

    ${currentTime} =  Get Current Date  UTC  result_format=epoch
    Wait for SAV policy on endpoint
    Wait For exclusion configuration on endpoint
    Wait For Scan Time configuration on endpoint  ${scan_time}

    # Wait up to 30 minutes:
    Wait For Central Scheduled Scan to start
    Wait For Central Scheduled Scan to complete
    Wait For Scheduled Scan Completion in central  ${currentTime}
    Wait For Eicar Detection in central  /tmp/testeicar/eicar.com  ${currentTime}

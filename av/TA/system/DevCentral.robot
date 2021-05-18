*** Settings ***
Documentation    Prove end-to-end management from Central

Library         ../Libs/CloudClient/CloudClient.py
Library         ../Libs/AVScanner.py
Library         ../Libs/LogUtils.py
Library         ../Libs/OnFail.py
Library         DateTime
Library         OperatingSystem
Library         String

Resource        ../shared/AVResources.robot
Resource        ../shared/AVAndBaseResources.robot

Suite Setup     DevCentral Suite Setup
Suite Teardown  DevCentral Suite Teardown

Test Setup      DevCentral Test Setup
Test Teardown   DevCentral Test TearDown

*** Variables ***

${MCSROUTER_LOG_PATH}   ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log

*** Keywords ***

DevCentral Suite Setup
    alter etc hosts  /etc/hosts  127.0.0.1 dci.sophosupd.com dci.sophosupd.net dlclssplavuc1

DevCentral Suite Teardown
    restore etc hosts

DevCentral Test Setup
    register on fail  Dump Log   ${SCANNOW_LOG_PATH}
    register on fail  Dump Log   ${THREAT_DETECTOR_LOG_PATH}
    register on fail  Dump Log   ${AV_LOG_PATH}
    register on fail  Dump Log   ${MCSROUTER_LOG_PATH}
    register on fail  Dump Log   ${MANAGEMENT_AGENT_LOG_PATH}
    register on fail  Dump Log   ${SOPHOS_INSTALL}/logs/base/suldownloader.log

DevCentral Test TearDown
    Run Teardown Functions

Install Base And Plugin Without Register
    Install With Base SDDS
    ## MCS router stopped

Create Eicar
    [Arguments]  ${path}
    Create File      ${path}    ${EICAR_STRING}

Wait for SAV policy on endpoint
    Wait Until Created  ${SOPHOS_INSTALL}/base/mcs/policy/SAV-2_policy.xml  5 mins

Wait for ALC policy on endpoint
    Wait Until Created  ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml  5 mins

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
    Wait until scheduled scan updated

Wait For Scan Time configuration on endpoint
    [Arguments]     ${scan_time}
    Wait For File With Particular Contents   <time>${scan_time}:00</time>  ${SOPHOS_INSTALL}/base/mcs/policy/SAV-2_policy.xml
    Wait until scheduled scan updated

Wait For Scan Now to start
    Wait Until AV Plugin Log Contains  Starting scan Scan Now   120

Wait For Central Scheduled Scan to start
    # Wait up to 32 minutes for the scan to start
    Wait Until AV Plugin Log Contains  Starting scan Sophos Cloud Scheduled Scan     1920

Wait For Scan Now to complete
    Wait Until AV Plugin Log Contains  Completed scan Scan Now

Wait For Central Scheduled Scan to complete
    Wait Until AV Plugin Log Contains  Completed scan Sophos Cloud Scheduled Scan

mcsrouter Log Contains
    [Arguments]  ${input}
    File Log Contains  ${MCSROUTER_LOG_PATH}   ${input}

Wait for mcsrouter to be running
    Wait Until File Log Contains  mcsrouter Log Contains    mcsrouter.computer <> Adding SAV adapter    timeout=15

*** Test Cases ***
#
#Test Time sources
#    [Tags]  MANUAL
#    [Documentation]  Investigate Get Current Date - see https://github.com/robotframework/robotframework/issues/3306
#    ${epoch_utc} =  Get Current Date  UTC  result_format=epoch
#    ${epoch_local} =  Get Current Date  result_format=epoch
#    ${current_utc} =  Get Current Date  UTC
#    ${current_local} =  Get Current Date
#    Evaluate Time Sources  ${epoch_utc}  ${epoch_local}  ${current_utc}  ${current_local}
#
#Scan now from Central and Verify Scan Completed and Eicar Detected
#    [Tags]  SYSTEM  CENTRAL
#    [Documentation]  Test that we can perform a scan now from Central (Dev/QA regions)
#    Select Central Region
#    log central events
#    clear alerts in central
#    Ensure AV Policy Exists
#    Install Base And Plugin Without Register
#    # Base will uninstall SSPL-AV if it does an update
#    Remove File  ${SOPHOS_INSTALL}/base/update/certs/ps_rootca.crt
#    Register In Central
#    Wait Until AV Plugin Log Contains  Starting scanScheduler
#    Wait for computer to appear in Central
#    Assign AntiVirus Product to Endpoint in Central
#    Wait for mcsrouter to be running
#    Configure Exclude everything else in Central  /tmp_test/testeicar/
#    Register Cleanup  Remove Directory  /tmp_test/testeicar   recursive=True
#    Create Eicar  /tmp_test/testeicar/eicar.com
#    Wait For exclusion configuration on endpoint
#    # See https://github.com/robotframework/robotframework/issues/3306
#    ${currentTime} =  Get Current Date  result_format=epoch
#    send scan now in central
#    Wait For Scan Now to start
#    Wait For Scan Now to complete
#    Wait For Scan Completion in central  ${currentTime}
#    Wait For Eicar Detection in central  /tmp_test/testeicar/eicar.com  ${currentTime}
#
#Scheduled Scan from Central and Verify Scan Completed and Eicar Detected
#    [Tags]  SYSTEM  CENTRAL  MANUAL
#    [Timeout]    40min
#    Select Central Region
#    log central events
#    clear alerts in central
#    Ensure AV Policy Exists
#    Install Base And Plugin Without Register
#    # Base will uninstall SSPL-AV if it does an update
#    Remove File  ${SOPHOS_INSTALL}/base/update/certs/ps_rootca.crt
#    Register In Central
#    Wait for computer to appear in Central
#    Assign AntiVirus Product to Endpoint in Central
#    Register Cleanup  Remove Directory  /tmp_test/testeicar   recursive=True
#    Create Eicar  /tmp_test/testeicar/eicar.com
#
#    # Includes Configure Exclude everything else in Central  /tmp_test/testeicar/
#    ${scan_time} =  Configure next available scheduled Scan in Central  /tmp_test/testeicar/
#
#    ${currentTime} =  Get Current Date  UTC  result_format=epoch
#    Wait for SAV policy on endpoint
#    Wait For exclusion configuration on endpoint
#    Wait For Scan Time configuration on endpoint  ${scan_time}
#
#    # Wait up to 30 minutes:
#    Wait For Central Scheduled Scan to start
#    Wait For Central Scheduled Scan to complete
#    Wait For Scheduled Scan Completion in central  ${currentTime}
#    Wait For Eicar Detection in central  /tmp_test/testeicar/eicar.com  ${currentTime}

SAV and ALC Policy Arrives And Is Handled Correctly
    [Tags]  SYSTEM  CENTRAL  MANUAL
    Select Central Region
    log central events
    clear alerts in central
    Ensure AV Policy Exists
    Install Base And Plugin Without Register
    Remove File  ${SOPHOS_INSTALL}/base/update/certs/ps_rootca.crt
    Register In Central
    Wait for computer to appear in Central
    Assign AntiVirus Product to Endpoint in Central

    Wait for ALC policy on endpoint
    Wait for SAV policy on endpoint

    #Expect error to show up either 0 times or once
    Run Keyword And Expect Error  *
    ...     LogUtils.Check Log Contains String N Times  times ${SOPHOS_INSTALL}/plugins/av/log/av.log  av.log  Failed to read customerID - using default value  2

    Stop AV Plugin
    Remove File    ${AV_LOG_PATH}
    Remove File    ${THREAT_DETECTOR_LOG_PATH}
    Start AV Plugin

    Wait Until AV Plugin Log exists   timeout=30

    Wait Until AV Plugin Log Contains  ALC policy received for the first time.
    Wait Until AV Plugin Log Contains  Processing ALC Policy
    Wait Until AV Plugin Log Contains  SAV policy received for the first time.
    Wait Until AV Plugin Log Contains  Processing SAV Policy
    Wait Until AV Plugin Log Contains  Received policy from management agent for AppId: ALC
    Wait until scheduled scan updated
    Wait Until AV Plugin Log Contains   Restarting sophos_threat_detector as the system configuration has changed
    #Expect error to show up either 0 times or once
    Run Keyword And Expect Error  *
    ...     Check Log Contains String N  times ${SOPHOS_INSTALL}/plugins/av/log/av.log  av.log  Failed to read customerID - using default value  2

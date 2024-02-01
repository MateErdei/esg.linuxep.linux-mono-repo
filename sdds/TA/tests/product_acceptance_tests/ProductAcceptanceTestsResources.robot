*** Settings ***
Resource    ${COMMON_TEST_ROBOT}/AVResources.robot
Resource    ${COMMON_TEST_ROBOT}/EDRResources.robot
Resource    ${COMMON_TEST_ROBOT}/EventJournalerResources.robot
Resource    ${COMMON_TEST_ROBOT}/LiveResponseResources.robot
Resource    ${COMMON_TEST_ROBOT}/RuntimeDetectionsResources.robot
#Resource    ${COMMON_TEST_ROBOT}/DeviceIsolationResources.robot
Resource    ${COMMON_TEST_ROBOT}/ResponseActionsResources.robot

*** Keywords ***
Wait for All Processes To Be Running
    Wait For Base Processes To Be Running
    Wait For EDR to be Installed
    Check Event Journaler Installed
    Check All Persistent Av Processes Are Started
    Wait For RuntimeDetections to be Installed
    Check Live Response Plugin Installed
#    Check Device Isolation Executable Running
    Check Response Actions Executable Running

Product Acceptance Test Setup
    Upgrade Resources SDDS3 Test Setup
    # TODO: Remove once LINUXDAR-8666 is fixed
    Register Cleanup  Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/watchdog.log  ProcessMonitoringImpl <> /opt/sophos-spl/base/bin/UpdateScheduler died with signal 9
    # TODO: Remove once LINUXDAR-8240 is fixed
    Register Cleanup  Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/watchdog.log  ProcessMonitoringImpl <> /opt/sophos-spl/base/bin/sophos_managementagent died with signal 9

*** Settings ***
Resource    AVResources.robot
Resource    EDRResources.robot
Resource    EventJournalerResources.robot
Resource    LiveResponseResources.robot
Resource    RuntimeDetectionsResources.robot

*** Keywords ***
Wait for All Processes To Be Running
    Wait For Base Processes To Be Running
    Wait For EDR to be Installed
    Check Event Journaler Installed
    Check AV Plugin Installed Directly
    Wait For RuntimeDetections to be Installed
    Check Live Response Plugin Installed

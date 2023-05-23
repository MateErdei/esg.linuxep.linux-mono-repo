*** Settings ***
Resource  ../av_plugin/AVResources.robot
Resource  ../edr_plugin/EDRResources.robot
Resource  ../event_journaler/EventJournalerResources.robot
Resource  ../runtimedetections_plugin/RuntimeDetectionsResources.robot
Resource  ../event_journaler/EventJournalerResources.robot
Resource  ../liveresponse_plugin/LiveResponseResources.robot

*** Keywords ***
Wait for All Processes To Be Running
    Wait For Base Processes To Be Running
    Wait For EDR to be Installed
    Check Event Journaler Installed
    Check AV Plugin Installed Directly
    Wait For RuntimeDetections to be Installed
    Check Live Response Plugin Installed

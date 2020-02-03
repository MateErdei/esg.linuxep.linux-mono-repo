*** Settings ***

Resource  ../installer/InstallerResources.robot
Resource  ../watchdog/LogControlResources.robot
Resource  ../scheduler_update/SchedulerUpdateResources.robot
Resource  MDRResources.robot
Resource  ../mcs_router-nova/McsRouterNovaResources.robot
Resource  ../GeneralTeardownResource.robot

Suite Setup  Require Installed

Test Setup     MDR Test Setup
Test Teardown  MDR Status Test Teardown

Default Tags   MDR_PLUGIN

*** Variables ***

${MDR_Disabled_File}  ${SOPHOS_INSTALL}/base/pluginRegistry/MTR_disabled


*** Test Cases ***

MDR Plugin Writes NoRef Status When No Policy Has Been Received
    Stop Management Agent Via WDCTL
    Start Management Agent Via WDCTL

    Wait for MDR Status

    ${MDRStatus} =  Get File  ${MDR_STATUS_XML}
    Should Contain  ${MDRStatus}   Res='NoRef'
    Should Contain  ${MDRStatus}   RevID=''

MDR Plugin Receives Invalid MDR Policy And Does Not Write Policy To Expected Location And Sends Failure Status
    Create File  ${SOPHOS_INSTALL}/tmp/MDR_policy.xml  junk
    Move File  ${SOPHOS_INSTALL}/tmp/MDR_policy.xml  ${SOPHOS_INSTALL}/base/mcs/policy

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  MDR Plugin Log Contains  Received new policy

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 sec
    ...  MDR Status Should Report Failure

    File Should Not Exist   ${MDRPluginPolicyLocation}

MDR Plugin Receives Invalid MDR Policy And Then Receives Valid Policy
    ${MDRLog} =  Set Variable  ${SOPHOS_INSTALL}/plugins/mtr/log/mtr.log
    ${ReceivedPolicyMessage} =  Set Variable  mtr <> Received new policy

    Create File  ${SOPHOS_INSTALL}/tmp/MDR_policy.xml  Junk
    Move File  ${SOPHOS_INSTALL}/tmp/MDR_policy.xml  ${SOPHOS_INSTALL}/base/mcs/policy

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  MDR Plugin Log Contains  ${ReceivedPolicyMessage}

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 sec
    ...  MDR Status Should Report Failure

    File Should Not Exist   ${MDRPluginPolicyLocation}

    Send Default MDR Policy And Check it is Written With Correct Permissions
    ${MdrLogContents} =   Get File  ${MDRLog}
    Should Contain X Times  ${MdrLogContents}  ${ReceivedPolicyMessage}  2  MDR Log should contain : "${ReceivedPolicyMessage}" twice

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 sec
    ...  MDR Status Should Contain  'TESTPOLICY1'  'Same'

MDR Plugin Receives Valid MDR Policy And Writes Policy To Expected Location And Sends Same Status When MDR Disabled
    Disable MDR Executable And Restart MDR Plugin

    Send Default MDR Policy And Check it is Written With Correct Permissions

    Wait for MDR Status

    ${MDRStatus} =  Get File  ${MDR_STATUS_XML}
    Should Contain  ${MDRStatus}   Res='Same'
    Should Contain  ${MDRStatus}   RevID='TESTPOLICY1'



*** Keywords ***

MDR Test Setup
    Install MDR Directly with Fake SophosMTR
    Check SophosMTR installed

MDR Status Test Teardown
    MDR Test Teardown
    Run Keyword If Test Failed  Dump Mcs Router Dir Contents
    Run Keyword And Ignore Error  Remove Directory  ${MDR_Disabled_File}  recursive=True
    Run Keyword And Ignore Error  Remove File  ${MDR_Disabled_File}
    Remove Files In Directory  ${SOPHOS_INSTALL}/base/mcs/policy
    Restart SSPL

MDR Status Should Contain
   [Arguments]  ${REVID}  ${RES}
    ${MDRStatus} =  Get File  ${MDR_STATUS_XML}
    Should Contain  ${MDRStatus}   Res=${RES}
    Should Contain  ${MDRStatus}   RevID=${REVID}

MDR Status Should Report Failure
    MDR Status Should Contain   ''   'Failure'

Restart SSPL
    Stop SSPL
    Sleep  3
    Start SSPL

Start SSPL
        ${stopResult} =  Run Process   systemctl start sophos-spl   shell=yes
        Should Be Equal As Integers   ${stopResult.rc}       0

Stop SSPL
        ${startResult} =  Run Process   systemctl stop sophos-spl   shell=yes
        Should Be Equal As Integers   ${startResult.rc}       0
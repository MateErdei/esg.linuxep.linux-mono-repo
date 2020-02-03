*** Settings ***
Library    ${libs_directory}/LogUtils.py
Library    ${libs_directory}/Watchdog.py
Library    ${libs_directory}/MCSRouter.py

Resource  ../installer/InstallerResources.robot
Resource  ../watchdog/LogControlResources.robot
Resource  ../scheduler_update/SchedulerUpdateResources.robot
Resource  MDRResources.robot
Resource  ../mcs_router-nova/McsRouterNovaResources.robot
Resource  ../GeneralTeardownResource.robot

Suite Setup     Run Keywords
...             Require Fresh Install  AND
...             Run Process  ${SOPHOS_INSTALL}/bin/wdctl  stop  mcsrouter

Suite Teardown  Require Uninstalled

Test Setup     MDR Test Setup
Test Teardown  Test Teardown

Default Tags   MDR_PLUGIN

*** Variables ***

${MDR_Disabled_File}  ${SOPHOS_INSTALL}/base/pluginRegistry/MTR_disabled
${PickYourPoisonExecutable}  SystemProductTestOutput/PickYourPoison
${PickYourPoisonArgFile}  /tmp/PickYourPoisonArgument
${MtrPluginExecutableName}  mtr
${MtrAgentExecutableName}  SophosMTR


*** Test Cases ***

MDR Plugin Receives Valid MDR Policy And Writes Policy To Expected Location And Sends Same Status
    Remove File   ${MDRPluginPolicyLocation}

    Send Default MDR Policy And Check it is Written With Correct Permissions

    Wait for MDR Status

    ${MDRStatus} =  Get File  ${MDR_STATUS_XML}
    Should Contain  ${MDRStatus}   Res='Same'
    Should Contain  ${MDRStatus}   RevID='TESTPOLICY1'


MDR Plugin Doesnt Start MDR Agent When Disabled File Exists And Starts It When The File Is Removed
    Disable MDR Executable And Restart MDR Plugin
    Stop MDR Plugin
    Remove File   ${MDR_Disabled_File}
    Start MDR Plugin
    Wait Until SophosMTR Executable Running

MDR Plugin Starts MDR Agent When Disabled Folder Exists
    Wait for MDR Executable To Be Running
    Stop MDR Plugin
    Create Directory   ${MDR_Disabled_File}
    Start MDR Plugin
    Wait Until SophosMTR Executable Running

MDR Will Log That It Is Restarting After A New Policy Is Received
    Wait for MDR Executable To Be Running
    Send Default MDR Policy And Check it is Written With Correct Permissions

    Wait Until Keyword Succeeds
    ...   10 secs
    ...   1 sec
    ...  Check Mdr Log Contains In Order  Run SophosMTR  Process main loop: Check output

    Send New MDR Policy

    Wait Until Keyword Succeeds
    ...   10 secs
    ...   1 sec
    ...  Check Mdr Log Contains In Order  Policy changed  Stopping SophosMTR  Restarting SophosMTR to apply new policy  Run SophosMTR

    Wait Until SophosMTR Executable Running

MDR Will Log That It Is Not Restarting Due To A Disable File After A New Policy Is Received
    Wait for MDR Executable To Be Running
    Send Default MDR Policy And Check it is Written With Correct Permissions

    Wait Until Keyword Succeeds
    ...   10 secs
    ...   1 sec
    ...  Check Mdr Log Contains In Order  Run SophosMTR  Process main loop: Check output

    Create File   ${MDR_Disabled_File}    content not used
    Send New MDR Policy

    Wait Until Keyword Succeeds
    ...   10 secs
    ...   1 sec
    ...  Check Mdr Log Contains In Order  Policy changed  Stopping SophosMTR  Not restarting to apply policy because SophosMTR is disabled
    # Sleep to give SophosMTR time to start (which we don't expect it to)
    Sleep  5
    Check SophosMTR Executable Not Running


Only One MDR Plugin May Be Running at a Time
    [Teardown]  Append Kill To Teardown
    Stop MDR Plugin
    ${mtrPlugin}=  Set Variable  ${MDR_PLUGIN_PATH}/bin/mtr
    ${first}=  Start Process  ${mtrPlugin}
    ${pid}=  Get Process Id  ${first}
    Report on Pid  ${pid}
    ${second}=  Start Process  ${mtrPlugin}
    ${pid}=  Get Process Id  ${second}
    Report on Pid  ${pid}
    ${result}=  Wait For Process  ${second}    timeout=30
    Should Not Be Equal As Integers  ${result.rc}  0  msg="Plugin is expected to report it failed to start: ${result.stderr}"
    MDR Plugin Log Contains  Only one instance of mtr can run.


*** Keywords ***

MDR Test Setup
    Require Installed
    Override LogConf File as Global Level  DEBUG
    Install MDR Directly with Fake SophosMTR

Append Kill To Teardown
    Terminate All Processes  kill=True
    Test Teardown

Test Teardown
    MDR Test Teardown
    Run Keyword If Test Failed  Dump Mcs Router Dir Contents
    Run Keyword And Ignore Error  Remove Directory  ${MDR_Disabled_File}  recursive=True
    Run Keyword And Ignore Error  Remove File  ${MDR_Disabled_File}
    Run Keyword And Ignore Error  Remove File  ${PickYourPoisonArgFile}

MDRLog Report Starting SophosMTR
    [Arguments]  ${times}
    ${MDRLog} =  Set Variable  ${SOPHOS_INSTALL}/plugins/mtr/log/mtr.log
    ${MdrLogContents} =   Get File  ${MDRLog}
    Should Contain X Times  ${MdrLogContents}  Run SophosMTR  ${times}  MDR Log should contain : "Run SophosMTR. Times=${times}"

Send New MDR Policy
    ${OriginalMDRPolicy} =  Get File  ./SupportFiles/CentralXml/MDR_policy.xml
    ${MDRPolicy} =  Replace String   ${OriginalMDRPolicy}  TESTPOLICY1   NEWPOLICY1
    Create File  ${SOPHOS_INSTALL}/tmp/MDR_policy.xml  ${MDRPolicy}
    Move File  ${SOPHOS_INSTALL}/tmp/MDR_policy.xml  ${SOPHOS_INSTALL}/base/mcs/policy
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check MDR Policy Has Been Written And RevID Logged  NEWPOLICY1
    ${MDRComparison} =  Get File  ${MDRPluginPolicyLocation}
    Should Be Equal  ${MDRPolicy}  ${MDRComparison}


*** Settings ***
Library    ${LIBS_DIRECTORY}/LogUtils.py
Library    ${LIBS_DIRECTORY}/Watchdog.py
Library    ${LIBS_DIRECTORY}/MCSRouter.py

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

Test Setup     Local Test Setup
Test Teardown  Test Teardown

Default Tags   MDR_PLUGIN  FAULTINJECTION

*** Variables ***

${PickYourPoisonExecutable}  ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}/PickYourPoison
${PickYourPoisonArgFile}  /tmp/PickYourPoisonArgument
${MtrPluginExecutableName}  mtr
${MtrAgentExecutableName}  SophosMTR

#arguments
${run}              --run
${spam}             --spam
${bigspam}          --bigspam
${numberSpam}        --num-spam
${deadlock}         --deadlock
${crash}            --crash


*** Test Cases ***

MDR Plugin Starts And Stops MDR Agent When Starting and Stopping
    [Tags]  SMOKE  MDR_PLUGIN  FAULTINJECTION
    Create Fake Sophos MDR Executable With Pick Your Poison  ${run}
    Wait for MDR Executable To Be Running
    Stop MDR Plugin
    Check Intended Fault Injection Argument Was Used  ${run}

MDR Agent Which Writes To Stdout And Stderr Repeatedly Does Not Cause MTR Plugin To Use Lots Of Memory Or Hang
    Create Fake Sophos MDR Executable With Pick Your Poison  ${spam}
    Wait for MDR Executable To Be Running
    ${memorySize1} =  Get Process Memory Usage  ${MtrPluginExecutableName}
    sleep  5
    ${memorySize2} =  Get Process Memory Usage  ${MtrPluginExecutableName}
    sleep  5
    ${memorySize3} =  Get Process Memory Usage  ${MtrPluginExecutableName}
    # check memory usage of mtr plugin has not increased
    All Should Be Equal  ${memorySize1}  ${memorySize2}  ${memorySize3}
    # Check that mtr plugin will stop
    Stop MDR Plugin
    Check Mdr Log Contains  This is stdout
    Check Mdr Log Contains  This is stderr
    Check Intended Fault Injection Argument Was Used  ${spam}

MTR Plugin Can Restart A Deadlocked MTR Executable
    Stop MDR Plugin
    Create Fake Sophos MDR Executable With Pick Your Poison  ${deadlock}
    #clear errors from when there was no MDR Executable
    Remove Mtr Plugin Log
    Start MDR Plugin
    Wait for MDR Executable To Be Running
    ${SophosMTRPid1} =  Get Process Pid  ${MtrAgentExecutableName}
    sleep  5
    Stop MDR Plugin
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Check Mdr Log Contains  Now in deadlock
    # give it a chance to leave deadlock (in case the fault injection is not working)
    sleep  5
    ChecK Mtr Log Does Not Contain  Out of deadlock
    Start MDR Plugin
    Wait for MDR Executable To Be Running
    ${SophosMTRPid2} =  Get Process Pid  ${MtrAgentExecutableName}
    Should Not Be Equal  ${SophosMTRPid1}  ${SophosMTRPid2}
    Check Intended Fault Injection Argument Was Used  ${deadlock}

MTR Plugin Backs Off When MTR Executable Does Not Have Execute Permissions
    [Tags]   MDR_PLUGIN  FAULTINJECTION
    Stop MDR Plugin
    Create Fake Sophos MDR Executable With Pick Your Poison  ${run}  000
    #clear errors from when there was no MDR Executable
    Remove Mtr Plugin Log
    Start MDR Plugin
    sleep  25
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check MTR Executable Starting Backs Off With Error  Unable to execute SophosMTR: Permission denied when executing ${SophosMDRExe}

MTR Plugin Backs Off When SophosMTR Is Not Really An Executable
    [Tags]   MDR_PLUGIN  FAULTINJECTION
    Stop MDR Plugin
    Create Executable Text File
    #clear errors from when there was no MDR Executable
    Remove Mtr Plugin Log
    Start MDR Plugin
    sleep  25
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check MTR Executable Starting Backs Off With Error  Exec format error when executing ${SophosMDRExe}

MTR Plugin Restarts Fake MTR Agent When It Stops Unexpectedly
    Create Fake Sophos MDR Executable With Pick Your Poison  ${run}
    Wait for MDR Executable To Be Running
    Kill SophosMTR Executable
    Wait for MDR Executable To Be Running
    Check Intended Fault Injection Argument Was Used  ${run}

MDR Plugin Restarts MDR Agent When It Stops On Its Own
    Create Fake Sophos MDR Executable With Pick Your Poison  ${crash}
    Wait for MDR Executable To Be Running
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check SophosMTR Executable Not Running
    Wait for MDR Executable To Be Running
    Check Intended Fault Injection Argument Was Used  ${crash}

Start MDR Plugin Without An MDR Executable Raises Error
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  MDR Plugin Log Contains  Executable does not exist at: ${SophosMDRExe}

MTR Plugin Reports Text Written to Standard Output of SophosMTR
    Create Fake Sophos MDR Executable With Pick Your Poison  ${run}
    Wait for MDR Executable To Be Running
    Kill SophosMTR Executable
    Wait Until Keyword Succeeds
    ...   10 secs
    ...   1 sec
    ...  MDR Plugin Log Contains  SophosMTR unexpected logs: Starting PickYourPoison Fault Injection Executable
    Wait for MDR Executable To Be Running
    Check Intended Fault Injection Argument Was Used  ${run}

MTR Plugin Reports Overflow of Standard Output from SophosMTR
    Create Fake Sophos MDR Executable With Pick Your Poison  ${numberSpam}
    Wait for MDR Executable To Be Running

    # demonstrate it captured stdout
    Wait Until Keyword Succeeds
    ...   10 secs
    ...   1 sec
    ...  MDR Plugin Log Contains  Non expected logs from SophosMTR: Starting PickYourPoison Fault Injection Executable

    # demonstrate it captured stderr output
    Wait Until Keyword Succeeds
    ...   10 secs
    ...   1 sec
    ...  MDR Plugin Log Contains  200\n201

    Kill SophosMTR Executable

    # if forced to stop, it will also log all the output of SophosMTR
    Wait Until Keyword Succeeds
    ...   20 secs
    ...   5 sec
    ...  MDR Plugin Log Contains  SophosMTR unexpected logs:

    Wait Until Keyword Succeeds
    ...   10 secs
    ...   1 sec
    ...  MDR Plugin Log Contains  Executable will keep Running

    Check Intended Fault Injection Argument Was Used  ${numberSpam}

*** Keywords ***

Create Executable Text File
    ${script} =     Set Variable  this is not an executable
    Create Directory   ${SOPHOS_INSTALL}/plugins/mtr/dbos/
    Create File   ${SophosMDRExe}    content=${script}
    Run Process  chmod  u+x  ${SophosMDRExe}

Create Fake Sophos MDR Executable With Pick Your Poison
    [Arguments]  ${arg}  ${permissions}=u+x
    Create Pick Your Poison Argument File  ${arg}
    Create Pick Your Poison Executable As SophosMDR  ${permissions}

Create Pick Your Poison Argument File
    [Arguments]  ${arg}
    Create File   ${PickYourPoisonArgFile}    content=${arg}

Create Pick Your Poison Executable As SophosMDR
    [Arguments]  ${permissions}=u+x
    Create Directory   ${SOPHOS_INSTALL}/plugins/mtr/dbos/
    Copy File  ${PickYourPoisonExecutable}  ${SophosMDRExe}
    ${result} =  Run Process  chmod  ${permissions}  ${SophosMDRExe}
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Strings  0  ${result.rc}

Local Test Setup
    Require Installed
    Override LogConf File as Global Level  DEBUG
    Install MDR Directly

Test Teardown
    MDR Test Teardown
    Run Keyword If Test Failed  Dump Mcs Router Dir Contents
    Run Keyword And Ignore Error  Remove File  ${PickYourPoisonArgFile}

Get Process Pid
    [Arguments]  ${executable}
    ${result} =  Run Process  pgrep   ${executable}
    Should Be Equal As Integers  ${result.rc}  0
    [Return]  ${result.stdout}

Get Process Memory Usage In Kbytes
    [Arguments]  ${pid}
    ${result} =  Run Process   ps -o rss\= ${pid}   shell=True
    Should Be Equal As Integers  ${result.rc}  0
    [Return]  ${result.stdout}

Get Process Memory Usage
    [Arguments]  ${executable}
    ${pid} =  Get Process Pid  ${executable}
    ${kbytes} =  Get Process Memory Usage In Kbytes  ${pid}
    [Return]  ${kbytes}

Check MTR Executable Starting Backs Off With Error
    [Arguments]  ${errorMessage}
    Check Mdr Log Contains In Order
    ...  ${errorMessage}
    ...  SophosMTR failed to start. Scheduling a retry in 10 seconds
    ...  ${errorMessage}
    ...  SophosMTR failed to start. Scheduling a retry in 20 seconds
    ...  ${errorMessage}
    ...  SophosMTR failed to start. Scheduling a retry in 40 seconds

Check Intended Fault Injection Argument Was Used
    [Arguments]  ${argument}
    # ensure Executable has stopped so we can get argument
    Stop MDR Plugin
    # The file system may take a small amount of time to flush to disk
    # and be available to others to see what the mdr had written to its log.
    Wait Until Keyword Succeeds
    ...   5 secs
    ...   1 sec
    ...   Check Mdr Log Contains  using argument: ${argument}
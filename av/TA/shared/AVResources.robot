*** Settings ***
Library         Process
Library         OperatingSystem
Library         String
Library         ../Libs/FakeManagement.py

Resource    ComponentSetup.robot

*** Variables ***
${COMPONENT}       av
${COMPONENT_UC}    AV
${AV_PLUGIN_PATH}  ${COMPONENT_ROOT_PATH}
${AV_PLUGIN_BIN}   ${COMPONENT_BIN_PATH}
${AV_LOG_PATH}     ${AV_PLUGIN_PATH}/log/${COMPONENT}.log
${BASE_SDDS}       ${TEST_INPUT_PATH}/${COMPONENT}/base-sdds/
${AV_SDDS}         ${COMPONENT_SDDS}
${PLUGIN_SDDS}     ${COMPONENT_SDDS}
${PLUGIN_BINARY}   ${SOPHOS_INSTALL}/plugins/${COMPONENT}/sbin/${COMPONENT}

*** Keywords ***
Run Shell Process
    [Arguments]  ${Command}   ${OnError}   ${timeout}=20s
    ${result} =   Run Process  ${Command}   shell=True   timeout=${timeout}
    Should Be Equal As Integers  ${result.rc}  0   "${OnError}.\n stdout: \n ${result.stdout} \n. stderr: \n ${result.stderr}"

Check Plugin Running
    Run Shell Process  pidof ${PLUGIN_BINARY}   OnError=AV not running

Check sophos_threat_detector Running
    Run Shell Process  pidof ${SOPHOS_INSTALL}/plugins/${COMPONENT}/sbin/sophos_threat_detector   OnError=sophos_threat_detector not running

Check AV Plugin Running
    Check Plugin Running

File Log Contains
    [Arguments]  ${path}  ${input}
    ${content} =  Get File   ${path}
    Should Contain  ${content}  ${input}

Wait Until File Log Contains
    [Arguments]  ${logCheck}  ${input}
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  ${logCheck}  ${input}

File Log Does Not Contain
    [Arguments]  ${logCheck}  ${input}
    Run Keyword And Expect Error
    ...  Keyword '${logCheck}' failed after retrying for 15 seconds.*does not contain '${input}'
    ...  Wait Until Keyword Succeeds
    ...    15 secs
    ...    1 secs
    ...    ${logCheck}  ${input}

AV Plugin Log Contains
    [Arguments]  ${input}
    File Log Contains  ${AV_LOG_PATH}   ${input}

Wait Until AV Plugin Log Contains
    [Arguments]  ${input}
    Wait Until File Log Contains  AV Plugin Log Contains   ${input}

AV Plugin Log Does Not Contain
    [Arguments]  ${input}
    File Log Does Not Contain  AV Plugin Log Contains  ${input}

Plugin Log Contains
    [Arguments]  ${input}
    File Log Contains  ${AV_LOG_PATH}   ${input}

FakeManagement Log Contains
    [Arguments]  ${input}
    File Log Contains  ${FAKEMANAGEMENT_AGENT_LOG_PATH}   ${input}

Management Log Contains
    [Arguments]  ${input}
    File Log Contains  ${MANAGEMENT_AGENT_LOG_PATH}   ${input}

Wait Until Management Log Contains
    [Arguments]  ${input}
    Wait Until File Log Contains  Management Log Contains   ${input}

Management Log Does Not Contain
    [Arguments]  ${input}
    File Log Does Not Contain  Management Log Contains  ${input}

Check Plugin Installed and Running
    File Should Exist   ${PLUGIN_BINARY}
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  3 secs
    ...  Check Plugin Running
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  3 secs
    ...  Plugin Log Contains  ${COMPONENT} <> Entering the main loop

Check AV Plugin Installed
    Check Plugin Installed and Running
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  3 secs
    ...  FakeManagement Log Contains   Registered plugin: ${COMPONENT}

Install With Base SDDS
    Remove Directory   ${SOPHOS_INSTALL}   recursive=True
    Directory Should Not Exist  ${SOPHOS_INSTALL}
    Install Base For Component Tests
    Install AV Directly from SDDS

Uninstall All
    Log File    /tmp/installer.log
    Log File   ${AV_LOG_PATH}
    Log File   ${SOPHOS_INSTALL}/logs/base/watchdog.log
    ${result} =   Run Process  bash ${SOPHOS_INSTALL}/bin/uninstall.sh --force   shell=True   timeout=20s
    Should Be Equal As Integers  ${result.rc}  0   "Failed to uninstall base.\n stdout: \n${result.stdout}\n. stderr: \n {result.stderr}"

Uninstall And Revert Setup
    Uninstall All
    Setup Base And Component

Install Base For Component Tests
    File Should Exist     ${BASE_SDDS}/install.sh
    Run Shell Process   bash -x ${BASE_SDDS}/install.sh 2> /tmp/installer.log   OnError=Failed to Install Base   timeout=60s
    Run Keyword and Ignore Error   Run Shell Process    /opt/sophos-spl/bin/wdctl stop mcsrouter  OnError=Failed to stop mcsrouter

Install AV Directly from SDDS
    ${result} =   Run Process  bash ${AV_SDDS}/install.sh   shell=True   timeout=20s
    Should Be Equal As Integers  ${result.rc}  0   "Failed to install plugin.\n stdout: \n${result.stdout}\n. stderr: \n {result.stderr}"

Check AV Plugin Installed With Base
    Check Plugin Installed and Running

Display All SSPL Files Installed
    ${handle}=  Start Process  find ${SOPHOS_INSTALL} | grep -v python | grep -v primarywarehouse | grep -v temp_warehouse | grep -v TestInstallFiles | grep -v lenses   shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}
    Log  ${result.stderr}

AV And Base Teardown
    Run Keyword If Test Failed   Log File   ${SOPHOS_INSTALL}/logs/base/watchdog.log
    Run Keyword If Test Failed   Log File   ${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log
    Run Keyword If Test Failed   Log File   ${SOPHOS_INSTALL}/logs/base/watchdog.log
    Run Keyword If Test Failed   Display All SSPL Files Installed
    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl stop av   OnError=failed to stop plugin
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Plugin Log Contains      av <> Plugin Finished
    Run Keyword If Test Failed   Log File   ${AV_LOG_PATH}
    Remove File    ${AV_LOG_PATH}
    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl start av   OnError=failed to start plugin

Create Install Options File With Content
    [Arguments]  ${installFlags}
    Create File  ${SOPHOS_INSTALL}/base/etc/install_options  ${installFlags}
    #TODO set permissions

Send Sav Policy To Base
    [Arguments]  ${policyFile}
    Copy File  ${RESOURCES_PATH}/${policyFile}  ${SOPHOS_INSTALL}/base/mcs/policy/SAV-2_policy.xml

Send Sav Action To Base
    [Arguments]  ${actionFile}
    ${savActionFilename}  Generate Random String
    Copy File  ${RESOURCES_PATH}/${actionFile}  ${SOPHOS_INSTALL}/base/mcs/action/SAV_action_${savActionFilename}.xml

Validate Scan Complete
    [Arguments]  ${XML}
    ${SCAN_COMPLETE_XML}  parse xml  ${SOPHOS_INSTALL}/base/mcs/event/2_event*
    ELEMENT TEXT SHOULD BE  source=${root}  expected=<scanComplete>  xpath=scanComplete

Configure Scan Exclusions Everything Else # Will allow for one directory to be selected during a scan
#TODO implementation required
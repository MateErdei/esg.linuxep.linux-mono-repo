*** Settings ***


Library     Process
Resource    ComponentSetup.robot
Resource    EDRResources.robot

Suite Setup     Install With Base SDDS
Suite Teardown  Uninstall All

Test Setup      No Operation
Test Teardown   EDR And Base Teardown
*** Variables ***
${OSQUERY_FLAGS_FILE}   ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.flags
*** Test Cases ***
EDR Plugin stops osquery when killed
    Check EDR Plugin Installed With Base
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check Osquery Running
    ${oldPid} =  Get Osquery pid
    ${result} =  Run Process  pgrep edr | xargs kill -9  shell=true
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check EDR Executable Not Running
    Check Osquery Running
    # wait for watchdog to bring edr back up
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check EDR Executable Running
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  File Should Contain  ${SOPHOS_INSTALL}/plugins/edr/log/edr.log  Stopping process
    Wait Until Keyword Succeeds
    ...  25 secs
    ...  1 secs
    ...  Check Osquery Has Restarted  ${oldPid}

EDR Plugin stops osquery when killed by segv
    Check EDR Plugin Installed With Base
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Osquery Running
    ${oldPid} =  Get Osquery pid
    ${result} =  Run Process  pgrep edr | xargs kill -11  shell=true
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check EDR Executable Not Running
    Check Osquery Running
    # wait for watchdog to bring edr back up
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check EDR Executable Running
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  File Should Contain  ${SOPHOS_INSTALL}/plugins/edr/log/edr.log  Stopping process
    Wait Until Keyword Succeeds
    ...  25 secs
    ...  1 secs
    ...  Check Osquery Has Restarted  ${oldPid}

EDR Plugin replaces flags file on startup
    Check EDR Plugin Installed With Base
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check Osquery Running
    Stop EDR
    Remove File   ${OSQUERY_FLAGS_FILE}
    Create File   ${OSQUERY_FLAGS_FILE}  content=badcontent
    Start EDR
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check EDR Executable Running
    Wait Until Keyword Succeeds
    ...  25 secs
    ...  5 secs
    ...  File Should Not Contain Only  ${OSQUERY_FLAGS_FILE}  badcontent

*** Keywords ***
Check Osquery Has Restarted
    [Arguments]  ${oldPid}
    ${newPid} =  Get Osquery Pid
    Should Not be Equal  ${oldPid}  ${newPid}
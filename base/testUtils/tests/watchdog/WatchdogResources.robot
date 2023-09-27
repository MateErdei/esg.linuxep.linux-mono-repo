*** Settings ***
Library    Process
Library    OperatingSystem
Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library    ${LIBS_DIRECTORY}/LogUtils.py
Library    ${LIBS_DIRECTORY}/Watchdog.py

Resource    ../management_agent/ManagementAgentResources.robot
Resource    ../GeneralTeardownResource.robot

*** Keywords ***
Stop System Watchdog
    ${result} =    Run Process     systemctl       stop        sophos-spl


Restart Product
    ${result} =  Run Process  systemctl  restart  sophos-spl
    Should Be Equal As Strings  0  ${result.rc}


Wdctl Stop Plugin
    [Arguments]  ${pluginName}
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   stop   ${pluginName}

    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Should Be Equal As Integers    ${result.rc}    0  Failed to stop plugin: ${pluginName}. Reason: ${result.stderr}

    Sleep  2 secs


Wdctl Start Plugin
    [Arguments]  ${pluginName}

    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   start   ${pluginName}

    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Should Be Equal As Integers    ${result.rc}    0  Failed to start plugin: ${pluginName}. Reason: ${result.stderr}


Uninstall And Stop Manual Watchdog
    Run Keyword If Test Failed  report wdctl log
    ${result} =  run process  ps -ef | grep sophos  shell=True
    Log    "ps stdout = ${result.stdout}"
    Log    "ps stderr = ${result.stderr}"
    Should Be Equal As Integers    ${result.rc}    0
    Kill Manual Watchdog
    Require Uninstalled


Report Wdctl Log
    ${contents} =  Get File   ${SOPHOS_INSTALL}/logs/base/wdctl.log
    Log     "wdctl.log = ${contents}"


Wdctl Test Teardown
    General Test Teardown
    Run Keyword If Test Failed  Dump All Processes
    Uninstall and Stop Manual Watchdog

Verify Watchdog Conf Has Changed
    [Arguments]    ${originalTime}
    ${latest}    Get Modified Time   ${WD_ACTUAL_USER_GROUP_IDS}
    Should Not Be Equal    ${originalTime}    ${latest}

Verify Watchdog Actual User Group ID File
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  verify_watchdog_config    expect_all_users=${True}

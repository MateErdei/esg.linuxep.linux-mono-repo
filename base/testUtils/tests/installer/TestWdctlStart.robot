*** Settings ***
Documentation    Test that wdctl start operation

Library    String

Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library    ${COMMON_TEST_LIBS}/LogUtils.py

Resource   ${COMMON_TEST_ROBOT}/GeneralTeardownResource.robot
Resource   ${COMMON_TEST_ROBOT}/InstallerResources.robot

Force Tags    TAP_PARALLEL4    WDCTL

*** Test Cases ***
Test wdctl times out if watchdog not running
    [Tags]    WATCHDOG
    Require Fresh Install
    Stop Watchdog
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl  start  foobar  timeout=1min
    Should Not Be Equal As Integers    ${result.rc}    ${-15}   "wdctl didn't timeout itself - was killed by robot"
    Should Not Be Equal As Integers    ${result.rc}    ${0}   "wdctl didn't report any error for timeout"

Test that wdctl can restart a plugin when the plugin configuration is updated while running
    Require Fresh Install
    ${old_content} =  Get File  ${SOPHOS_INSTALL}/base/pluginRegistry/tscheduler.json
    ${new_content} =  Replace String  ${old_content}  base/bin/tscheduler  base/bin/newTscheduler
    Log  ${new_content}
    Create File  ${SOPHOS_INSTALL}/tmp/tscheduler.json  content=${new_content}
    Copy File  ${SOPHOS_INSTALL}/base/bin/tscheduler  ${SOPHOS_INSTALL}/base/bin/newTscheduler
    Run Process  chmod  750   ${SOPHOS_INSTALL}/base/bin/newTscheduler
    Run Process  chown  root:sophos-spl-group  ${SOPHOS_INSTALL}/base/bin/newTscheduler
    ${copyresult} =    Run Process     ${SOPHOS_INSTALL}/bin/wdctl  copyPluginRegistration  ${SOPHOS_INSTALL}/tmp/tscheduler.json  timeout=1min
    ${startresult} =    Run Process     ${SOPHOS_INSTALL}/bin/wdctl  start  tscheduler  timeout=1min
    Wait Until Keyword Succeeds
        ...  30
        ...  5
        ...  Check Telemetry Scheduler Copy Is Running

    Check Watchdog Log Contains  Plugin info changed while plugin running so stopping plugin

Test that wdctl can reload a plugin configuration when started
    Require Fresh Install
    ${old_content} =  Get File  ${SOPHOS_INSTALL}/base/pluginRegistry/tscheduler.json
    ${new_content} =  Replace String  ${old_content}  base/bin/tscheduler  base/bin/newTscheduler
    Log  ${new_content}
    Create File  ${SOPHOS_INSTALL}/tmp/tscheduler.json  content=${new_content}
    Copy File  ${SOPHOS_INSTALL}/base/bin/tscheduler  ${SOPHOS_INSTALL}/base/bin/newTscheduler
    Run Process  chmod  750   ${SOPHOS_INSTALL}/base/bin/newTscheduler
    Run Process  chown  root:sophos-spl-group  ${SOPHOS_INSTALL}/base/bin/newTscheduler
    ${stopresult} =    Run Process     ${SOPHOS_INSTALL}/bin/wdctl  stop  tscheduler  timeout=1min
    Wait Until Keyword Succeeds
        ...  10
        ...  1
        ...  Check Telemetry Scheduler Plugin Not Running
    ${copyresult} =    Run Process     ${SOPHOS_INSTALL}/bin/wdctl  copyPluginRegistration  ${SOPHOS_INSTALL}/tmp/tscheduler.json  timeout=1min
    ${startresult} =    Run Process     ${SOPHOS_INSTALL}/bin/wdctl  start  tscheduler  timeout=1min
    Wait Until Keyword Succeeds
        ...  30
        ...  5
        ...  Check Telemetry Scheduler Copy Is Running

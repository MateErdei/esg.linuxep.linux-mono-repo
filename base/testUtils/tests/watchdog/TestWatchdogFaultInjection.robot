*** Settings ***
Documentation   Test wdctl can ask watchdog to restart a process

Library    Process
Library    OperatingSystem
Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library    ${LIBS_DIRECTORY}/Watchdog.py

Resource  ../installer/InstallerResources.robot
Resource  WatchdogResources.robot

Test Setup  Require Fresh Install

*** Test Cases ***

Test wdctl and Watchdog Can Handle A Plugin That cannot Be Executed And Logs Error
    [Tags]    WATCHDOG  WDCTL  FAULTINJECTION
    Remove File  /tmp/TestWdctlCanAddANewPluginToRunningWatchdog
    Remove File  ${SOPHOS_INSTALL}/base/pluginRegistry/testplugin.json
    Remove File  /tmp/TestWdctlHandleBrokenPlugin
    Remove File  ${SOPHOS_INSTALL}/base/pluginRegistry/testbrokenplugin.json

    Setup Test Plugin Config  echo "Plugin started at $(date)" >>/tmp/TestWdctlCanAddANewPluginToRunningWatchdog   ${TEMPDIR}  testplugin
    Setup Test Plugin Config  echo "Plugin started at $(date)" >>/tmp/TestWdctlHandleBrokenPlugin                  ${TEMPDIR}  testbrokenplugin   testbrokenplugin.sh
    ## call wdctl to copy configuration

    ${result1} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   copyPluginRegistration    ${TEMPDIR}/testbrokenplugin.json
    ${result2} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   copyPluginRegistration    ${TEMPDIR}/testplugin.json

    # break plugin by preventing it from being able to execute.
    Run Process     chmod   -x     ${SOPHOS_INSTALL}/testbrokenplugin.sh

    ## call wdctl to start plugin
    ${result1} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   start    testbrokenplugin
    ${result2} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   start    testplugin

    ## Verify testplugin has been run when another plugin is broken.  Proves system has not crashed.
    Wait For Marker To Be Created  /tmp/TestWdctlCanAddANewPluginToRunningWatchdog  30

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  3 secs
    ...  Check Watchdog Detect Broken Plugins

Watchdog Does Not Throw Unhandled Exception When Machine Id File Is Not Present At Startup
    ${result} =  Run Process  chmod  600  ${SOPHOS_INSTALL}/base/etc/machine_id.txt
    Should Be Equal As Strings  0  ${result.rc}
    ${result} =  Run Process  chown  root:root  ${SOPHOS_INSTALL}/base/etc/machine_id.txt
    Should Be Equal As Strings  0  ${result.rc}
    ${result} =  Run Process  systemctl  restart  sophos-spl
    Should Be Equal As Strings  0  ${result.rc}

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check Updatescheduler Log Contains  Error, Failed to read file: '/opt/sophos-spl/base/etc/machine_id.txt', file does not exist

*** Keywords ***
Check Watchdog Detect Broken Plugins
    # Check logs contain expected entries.
    ${wdctlLog}=  Get File  ${SOPHOS_INSTALL}/logs/base/wdctl.log
    # Check attempt was made to start the broken plugin
    Should Contain  ${wdctlLog}  Attempting to start testbrokenplugin
    # Check attempt was made to start the working plugin
    Should Contain  ${wdctlLog}  Attempting to start testplugin

    ${WatchdogLog}=  Get File  ${SOPHOS_INSTALL}/logs/base/watchdog.log
    # Check to ensure the broken plugin did not start
    Should Contain  ${WatchdogLog}  /opt/sophos-spl/testbrokenplugin.sh died
    # Additional Check to ensure the working test plugin started and run then exited.
    Should Contain  ${WatchdogLog}  /opt/sophos-spl/testPlugin.sh exited

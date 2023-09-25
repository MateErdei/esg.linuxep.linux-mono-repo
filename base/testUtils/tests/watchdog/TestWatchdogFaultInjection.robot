*** Settings ***
Documentation   Test wdctl can ask watchdog to restart a process

Library    Process
Library    OperatingSystem
Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library    ${LIBS_DIRECTORY}/Watchdog.py
Library    ${COMMON_TEST_LIBS}/LogUtils.py

Resource  ../installer/InstallerResources.robot
Resource  WatchdogResources.robot


*** Test Cases ***

Test wdctl and Watchdog Can Handle A Plugin That cannot Be Executed And Logs Error
    [Tags]    WATCHDOG  WDCTL  FAULTINJECTION
    [Teardown]  Clean Up Files
    Require Fresh Install

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

Test wdctl and Watchdog aborts a plugin that will not shutdown cleanly
    # Exclude on SLES12 until LINUXDAR-7121 is fixed
    [Tags]    WATCHDOG  WDCTL  FAULTINJECTION    EXCLUDE_SLES12
    [Teardown]  Clean Up Files
    Set Environment Variable  SOPHOS_CORE_DUMP_ON_PLUGIN_KILL  1
    Require Fresh Install

   setup_test_plugin_config_with_given_executable  SystemProductTestOutput/ignoreSignals
    ## call wdctl to copy configuration


    ${result2} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   copyPluginRegistration    ${TEMPDIR}/testplugin.json


    ${result2} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   start    fakePlugin
    sleep  2
    ${result2} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   stop    fakePlugin
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  2 secs
    ...  check_watchdog_log_contains  /opt/sophos-spl/testPlugin died with
    check_watchdog_log_contains  Killing process with abort signal
    check_watchdog_log_does_not_contain  /opt/sophos-spl/testPlugin died with 15


Watchdog Does Not Throw Unhandled Exception When Machine Id File Is Not Present At Startup
    Require Fresh Install
    ${result} =  Run Process  chmod  600  ${SOPHOS_INSTALL}/base/etc/machine_id.txt
    Should Be Equal As Strings  0  ${result.rc}
    ${result} =  Run Process  chown  root:root  ${SOPHOS_INSTALL}/base/etc/machine_id.txt
    Should Be Equal As Strings  0  ${result.rc}

    ${mark} =  Mark Update Scheduler Log

    ${result} =  Run Process  systemctl  restart  sophos-spl
    Should Be Equal As Strings  0  ${result.rc}

    Wait For Log Contains From Mark    ${mark}    Error, Failed to read file: '/opt/sophos-spl/base/etc/machine_id.txt', permission denied
    ...  timeout=${10}

*** Keywords ***
Clean Up Files
    General Test Teardown
    Remove Environment Variable  SOPHOS_CORE_DUMP_ON_PLUGIN_KILL
    Remove File  /tmp/TestWdctlCanAddANewPluginToRunningWatchdog
    Remove File  /tmp/TestWdctlHandleBrokenPlugin
    Remove File  ${TEMPDIR}/testplugin.json
    Remove File  ${TEMPDIR}/testbrokenplugin.json

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

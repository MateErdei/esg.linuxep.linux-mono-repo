*** Settings ***
Documentation    Suite description
Library     ${libs_directory}/LogUtils.py


Resource  ../installer/InstallerResources.robot
Resource  ../GeneralTeardownResource.robot
Resource  ../watchdog/LogControlResources.robot
Resource  MDRResources.robot
Suite Setup  Require Installed
Test Setup     Test Setup
Test Teardown  Test Teardown

Default Tags   MDR_PLUGIN   MANAGEMENT_AGENT

*** Variables ***
${OSQUERYPATH}        ${MDR_PLUGIN_PATH}/osquery/bin/osquery

*** Test Cases ***

Test Install From Fake Component Suite And SophosMDR Starts and Stops Correctly
    Block Connection Between EndPoint And FleetManager
    Install Directly From Component Suite
    Check MDR Component Suite Installed Correctly
    Insert MTR Policy

    File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/dbos/data/VERSION.ini
    File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/dbos/etc/licenses.txt
    Component Suite Version Ini File Contains Proper Format   ${SOPHOS_INSTALL}/plugins/mtr/dbos/data/VERSION.ini


    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/var/policy/mtr.xml
    Wait Until SophosMTR Executable Running  20
    Wait Until Keyword Succeeds
    ...  35 secs
    ...  1 secs
    ...  Check Osquery Executable Running

    # sleep for five sconds as golang seems to take some time before it can kill a process
    sleep  5
    ${result} =    Run Process   ${SOPHOS_INSTALL}/bin/wdctl  stop  mtr

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check MDR Plugin Not Running

    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Check SophosMTR Executable Not Running

    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check Osquery Executable Not Running

Test That Killing SophosMTR Does Not Cause It To Hang
    Override LogConf File as Global Level   DEBUG
    Block Connection Between EndPoint And FleetManager
    Install Directly From Component Suite
    Check MDR Component Suite Installed Correctly
    Insert MTR Policy
    Wait for MDR Executable To Be Running
    Wait Until Keyword Succeeds
    ...  35 secs
    ...  1 sec
    ...  Check Osquery Executable Running

    # given that kill issue the -9 to the sophosmtr
    # it will leave the previous children running ( osquery)
    Kill SophosMTR Executable
    Check Osquery Executable Running

    Wait for MDR Executable To Be Running

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  3 secs
    ...  MDR Plugin Log Contains  Stopping process (osquery)
    Uninstall MDR Plugin
    Wait Until Keyword Succeeds  5 secs  1 secs  Check Osquery Executable Not Running


Remove Plugin Registration Stops All Sophos MTR Processes
    [Teardown]  Append Kill To Teardown
    Setup of Tests for Checkup MTR Remove Previously Running Processes

    Start MDR Plugin
    Wait Until Keyword Succeeds
    ...   35
    ...   5
    ...   Check Osquery Executable Running
    sleep  2 secs

    ${result}=  Run Process  /opt/sophos-spl/bin/wdctl  removePluginRegistration  mtr
    Run Keyword If   ${result.rc} != 0  Log  "Result: ${result.rc}. Stdout: ${result.stdout}. Stderr: ${result.stderr}"

    Wait Until Keyword Succeeds
    ...   15
    ...   5
    ...   Check SophosMTR Executable Not Running

    # We can't guarantee that OSquery will be stopped by the removePluginRegistration command so we don't check.
    # This is accepted behaviour (LINUXDAR-883)


MDR Plugin Stops Any Previously Running SophosMTR before starting another
    [Teardown]  Append Kill To Teardown
    Setup of Tests for Checkup MTR Remove Previously Running Processes
    ${handle}=  Start Process  ${SophosMDRExe}
    ${pid}=  Get Process Id  ${handle}
    Report on Pid  ${pid}
    #SophosMTR cannot receive stop signal until it has fully started up
    Sleep  5
    Start MDR Plugin
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill

    #SophosMTR returns 0 even when it receives kill signal.
    Should Be Equal As Integers  ${result.rc}  0  msg="SophosMTR was expected to be forced to shutdown: ${result.stdout} ${result.stderr}"

    Wait Until Keyword Succeeds
    ...  15
    ...  5
    ...  MDR Plugin Log Contains  Run SophosMTR

    # the first log demonstrate that it detected the old SophosMTR
    MDR Plugin Log Contains  Stopping process (SophosMTR)

MDR Plugin Stops Any Previously Running osquery before starting another
    [Teardown]  Append Kill To Teardown
    Setup of Tests for Checkup MTR Remove Previously Running Processes

    ${handle}=  Start Process  ${OSQUERYPATH}
    ${pid}=  Get Process Id  ${handle}
    Report on Pid  ${pid}

    Start MDR Plugin
    ${result}=  Wait For Process  ${handle}  timeout=30
    Should Not Be Equal As Integers  ${result.rc}  0  msg="Osquery was expected to be forced to shutdown: ${result.stdout} ${result.stderr}"

    Wait Until Keyword Succeeds
    ...  15
    ...  5
    ...  MDR Plugin Log Contains  Stopping process (osquery)

    Wait Until Keyword Succeeds
    ...  15
    ...  5
    ...  MDR Plugin Log Contains  Run SophosMTR


*** Keywords ***
Test Teardown
    MDR Test Teardown
    Remove Directory   ${COMPONENT_TEMP_DIR}  recursive=true

Test Setup
    Create Directory   ${COMPONENT_TEMP_DIR}

Append Kill To Teardown
    Test Teardown
    Terminate All Processes  kill=True

Setup of Tests for Checkup MTR Remove Previously Running Processes
    Block Connection Between EndPoint And FleetManager
    Install Directly From Component Suite
    Check MDR Component Suite Installed Correctly
    Insert MTR Policy
    Wait for MDR Executable To Be Running
    Stop MDR Plugin
    Remove File  ${MDR_LOG_FILE}

*** Settings ***
Library   OperatingSystem

Resource  ../installer/InstallerResources.robot
Resource  CommsComponentResources.robot
Library   ${LIBS_DIRECTORY}/CommsComponentUtils.py
Library   ${LIBS_DIRECTORY}/LogUtils.py
Library   ${LIBS_DIRECTORY}/OSUtils.py

Default Tags  COMMS
Test Setup  Test Setup
Test Teardown  Test Teardown


*** Variables ***
${TestReadOnlyMount}        ${SOPHOS_INSTALL}/var/sophos-spl-comms/test-ro-mount
${CommsNetworkLogsPath}     ${SOPHOS_INSTALL}/logs/base/sophos-spl-comms/comms_network.log
${CommsLocalLogsPath}     ${SOPHOS_INSTALL}/logs/base/sophosspl/comms_component.log
${CommsJailPath}        ${SOPHOS_INSTALL}/var/sophos-spl-comms

*** Test Cases ***

Test Comms Component Starts
    [Tags]   COMMS  TAP_TESTS
    Require Installed
    File Should Exist  ${SOPHOS_INSTALL}/base/bin/CommsComponent

    Check Watchdog Starts Comms Component
    Check Comms Component Log Does Not Contain Error

    File Exists With Permissions  ${SOPHOS_INSTALL}/logs/base/sophosspl/comms_component.log  sophos-spl-local  sophos-spl-group  -rw-------
    File Exists With Permissions  ${SOPHOS_INSTALL}/logs/base/sophos-spl-comms/comms_network.log  sophos-spl-network  sophos-spl-group  -rw-------

#FIXME LINUXDAR-2183: After downgrade is updated to run uninstall consider changing the test and watchdog code
Test CommsComponent Produces Mounts That Should Be Cleared When Watchdog Stops
    [Tags]   COMMS  TAP_TESTS
    Require Installed
    Wait Until Keyword Succeeds
    ...  3 secs
    ...  1 sec
    ...  Check Management Agent Running

    Check Comms Component Is Running
    Directory Should Exist     ${CommsJailPath}/usr/lib/systemd/

    Stop Watchdog
    Wait Until Keyword Succeeds
    ...  3 secs
    ...  1 sec
    ...  Directory Should Not Exist     ${CommsJailPath}/usr/lib/systemd/

    Verify All Mounts Have Been Removed  jailPath=${CommsJailPath}
    Check Log Contains  Successfully read only mounted   ${CommsNetworkLogsPath}   network log
    Check Log Contains  Waiting for network process to finish  ${CommsLocalLogsPath}  local log


Test Comms Component Will Not Launch If Chroot Directory Is Not Empty
    [Tags]   COMMS  TAP_TESTS
    Require Installed
    Check Comms Component Is Running

    Directory Should Exist     ${CommsJailPath}/usr/lib/systemd/
    #unmount an unexpected directory read only to prevent comms from starting
    Create Read Only Mount Inside Comms Chroot Path

    Stop Watchdog
    Check Comms Component Not Running

    #watch dog should have cleared the mounts
    Wait Until Keyword Succeeds
    ...  3 secs
    ...  1 sec
    ...  Directory Should Not Exist     ${CommsJailPath}/usr/lib/systemd/

    Start Watchdog
    Check Comms Component Not Running

    #unmount stray file
    Unmount Test Directory
    #wait for comms to run
    Wait Until Keyword Succeeds
        ...  20 secs
        ...  2 secs
        ...  Check Comms Component Is Running


Test Comms Component Backsup and Restore Logs
    [Tags]   COMMS  TAP_TESTS
    Require Installed

    Check Comms Component Is Running

    Wait Until Keyword Succeeds
        ...  10 secs
        ...  2 secs
        ...  Check Log Contains String N Times  ${CommsNetworkLogsPath}  comms-network.log   Successfully read only mounted '/lib' to path: '/opt/sophos-spl/var/sophos-spl-comms/lib  1

    #restart with systemctl
    Stop Watchdog
    Check Comms Component Not Running
    Start Watchdog
    #check logs restore and retain from previous start
    #each start is mounting the dependencies afresh
    Wait Until Keyword Succeeds
        ...  10 secs
        ...  2 secs
        ...  Check Log Contains String N Times   ${CommsNetworkLogsPath}   comms-network.log   Successfully read only mounted '/lib' to path: '/opt/sophos-spl/var/sophos-spl-comms/lib  2

    #restart with wdctl
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  stop   commscomponent
    Check Comms Component Not Running
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  start  commscomponent

    Wait Until Keyword Succeeds
        ...  20 secs
        ...  2 secs
        ...  Check Log Contains String N Times   ${CommsNetworkLogsPath}   comms-network.log   Successfully read only mounted '/lib' to path: '/opt/sophos-spl/var/sophos-spl-comms/lib  3
    Check Comms Component Log Does Not Contain Error

*** Keywords ***
Test Setup
    Require Uninstalled

Test Teardown
    General Test Teardown
    Require Uninstalled
    Unmount Test Directory

Create Read Only Mount Inside Comms Chroot Path
    Run Process  mkdir  -p  ${TestReadOnlyMount}
    ${mountSourceDirBase} =  Set Variable  /tmp/testMount
    ${mountFullTree} =  Set Variable  ${mountSourceDirBase}/testMountSubDirect
    Run Process  mkdir  -p  ${mountFullTree}
    Run Process  mount  -t  none  -o  bind,ro  ${mountSourceDirBase}  ${TestReadOnlyMount}
    Directory Should Exist  ${TestReadOnlyMount}/testMountSubDirect


Unmount Test Directory
    Run Process  umount  ${TestReadOnlyMount}
    Run Process  rm  -rf  ${TestReadOnlyMount}
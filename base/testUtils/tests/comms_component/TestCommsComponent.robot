*** Settings ***
Library   OperatingSystem

Resource  ../installer/InstallerResources.robot
Resource  CommsComponentResources.robot
Resource  ../watchdog/LogControlResources.robot
Library   ${LIBS_DIRECTORY}/CommsComponentUtils.py
Library   ${LIBS_DIRECTORY}/LogUtils.py
Library   ${LIBS_DIRECTORY}/OSUtils.py

Default Tags  COMMS
Test Setup  Test Setup
Test Teardown  Test Teardown


*** Variables ***
${TestReadOnlyMount}  ${SOPHOS_INSTALL}/var/sophos-spl-comms/test-ro-mount
${CommsNetworkLogsPath}  ${SOPHOS_INSTALL}/logs/base/sophos-spl-comms/comms_network.log

*** Test Cases ***

Test Comms Component Starts
    [Tags]   _COMMS  TAP_TESTS
    Require Installed
    File Should Exist  ${SOPHOS_INSTALL}/base/bin/CommsComponent

    Check Watchdog Starts Comms Component
    Check Comms Component Log Does Not Contain Error

    File Exists With Permissions  ${SOPHOS_INSTALL}/logs/base/sophosspl/comms_component.log  sophos-spl-local  sophos-spl-group  -rw-------
    File Exists With Permissions  ${SOPHOS_INSTALL}/logs/base/sophos-spl-comms/comms_network.log  sophos-spl-network  sophos-spl-network  -rw-------


Test Comms Component Starts With Process Running Inside And OutSide The Chroot From A Mounted Directory
    [Tags]   COMMS  TAP_TESTS
    Require Installed

    Check Comms Component Is Running

    # Create simple execuable script to run within chroot mount and outside chroot mount.
    ${script} =     Catenate    SEPARATOR=\n
    ...    \#!/bin/bash
    ...   sleep 300
    ...    \
    Create File    ${SOPHOS_INSTALL}/base/mcs/certs/sleep.sh   content=${script}
    Run Process  chmod  +x  ${SOPHOS_INSTALL}/base/mcs/certs/sleep.sh

    # Start the test sleep process from within a mount
    Start Process  sudo -H -u sophos-spl-network bash -c '${SOPHOS_INSTALL}/var/sophos-spl-comms/base/mcs/certs/sleep.sh'  shell=yes

    Stop Watchdog
    Check Comms Component Not Running

    Start Watchdog
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  Check Comms Component Is Running

    # Start the test sleep process from outside the chroot but from a direct which gets mounted
    Start Process  sudo -H -u sophos-spl-network bash -c '${SOPHOS_INSTALL}/base/mcs/certs/sleep.sh'  shell=yes

    Stop Watchdog
    Check Comms Component Not Running

    Start Watchdog
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  Check Comms Component Is Running



Test Comms Component Starts With Process Running Inside Chroot Not From A Mounted Directory
    [Tags]   COMMS  TAP_TESTS
    Require Installed

    Check Comms Component Is Running

    # Create simple execuable script to run inside the chroot not from a mounted directory
    ${script} =     Catenate    SEPARATOR=\n
    ...    \#!/bin/bash
    ...   sleep 300
    ...    \
    Create File    ${SOPHOS_INSTALL}/var/sophos-spl-comms/sleep.sh   content=${script}
    Run Process  chmod  +x  ${SOPHOS_INSTALL}/var/sophos-spl-comms/sleep.sh

    # Start the test sleep process from within a mount
    Start Process  sudo -H -u sophos-spl-network bash -c '${SOPHOS_INSTALL}/var/sophos-spl-comms/sleep.sh'  shell=yes

    Stop Watchdog
    Check Comms Component Not Running

    Start Watchdog
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  Check Comms Component Is Running

Test Comms Component Will Not Launch If Chroot Directory Is Not Empty
    [Tags]   COMMS  TAP_TESTS
    Require Installed

    Check Comms Component Is Running

    #unmount an unexpected directory read only to prevent comms from starting
    Create Read Only Mount Inside Comms Chroot Path

    Stop Watchdog
    Check Comms Component Not Running
    Directory Should Exist     ${SOPHOS_INSTALL}/var/sophos-spl-comms/usr/lib/systemd/
    Start Watchdog
    Check Comms Component Not Running

    #demonstrate that the start comms will cleanup and unmount known paths left in previous run
    Wait Until Keyword Succeeds
        ...  10 secs
        ...  2 secs
        ...  Directory Should Not Exist     ${SOPHOS_INSTALL}/var/sophos-spl-comms/usr/lib/systemd/

    #unmount stray file
    Unmount Test Directory
    #wait for comms to run
    Wait Until Keyword Succeeds
        ...  20 secs
        ...  2 secs
        ...  Check Comms Component Is Running

Test Comms Component Loggings
    [Tags]   COMMS  TAP_TESTS
    Require Installed

    #restart with systemctl
    Stop Watchdog
    Check Comms Component Not Running
    Override LogConf File as Global Level  DEBUG
    Start Watchdog

    Wait Until Keyword Succeeds
        ...  10 secs
        ...  2 secs
        ...  Check Comms Component Is Running

    Check Log Contains In Order
    ...  ${CommsNetworkLogsPath}
    ...  DEBUG
    ...  Unmount path: /opt/sophos-spl/var/sophos-spl-comms/lib
    ...  DEBUG
    ...  Unmount path: /opt/sophos-spl/var/sophos-spl-comms/usr/lib
    Check Log Contains In Order
    ...  ${CommsNetworkLogsPath}
    ...  DEBUG
    ...  Successfully read only mounted '/lib' to path: '/opt/sophos-spl/var/sophos-spl-comms/lib
    ...  DEBUG
    ...  Successfully read only mounted '/usr/lib' to path: '/opt/sophos-spl/var/sophos-spl-comms/usr/lib

Test Comms Component Backsup and Restores Logs
    [Tags]   COMMS  TAP_TESTS
    Require Installed

    #restart with systemctl
    Stop Watchdog
    Check Comms Component Not Running
    Override LogConf File as Global Level  DEBUG
    Start Watchdog

    Wait Until Keyword Succeeds
        ...  10 secs
        ...  2 secs
        ...  Check Comms Component Is Running

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


Test Comms Component Recovers From Previous Logs Backsup Failures
    [Tags]   COMMS  TAP_TESTS
    Require Installed

    Stop Watchdog
    Check Comms Component Not Running
    Override LogConf File as Global Level  DEBUG
    Start Watchdog
    Wait Until Keyword Succeeds
        ...  10 secs
        ...  2 secs
        ...  Check Comms Component Is Running
    Wait Until Keyword Succeeds
        ...  10 secs
        ...  2 secs
        ...  Check Log Contains String N Times  ${CommsNetworkLogsPath}  comms-network.log   Successfully read only mounted '/lib' to path: '/opt/sophos-spl/var/sophos-spl-comms/lib  1

    #restart with systemctl
    Stop Watchdog
    Check Comms Component Not Running

    #create previous backup file thats not been cleaned up
    ${comms_backup}=  Set Variable  ${SOPHOS_INSTALL}/tmp/comms_network_backup
    Create Directory And Setup Permissions   ${comms_backup}  sophos-spl-local   sophos-spl-group
    Copy File  ${CommsNetworkLogsPath}    ${comms_backup}
    Start Watchdog

    #check logs restore and retain from previous start
    #each start is mounting the dependencies afresh
    Wait Until Keyword Succeeds
        ...  10 secs
        ...  2 secs
        ...  Check Log Contains String N Times   ${CommsNetworkLogsPath}   comms-network.log   Successfully read only mounted '/lib' to path: '/opt/sophos-spl/var/sophos-spl-comms/lib  2

    Directory Should Not Exist  ${comms_backup}


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
*** Settings ***
Test Teardown    Install Tests Teardown

Suite Setup  Local Suite Setup

Documentation    Test Installation

Library    ${LIBS_DIRECTORY}/LogUtils.py
Library    ${LIBS_DIRECTORY}/OSUtils.py
Library    ${LIBS_DIRECTORY}/TemporaryDirectoryManager.py
Library    Collections

Resource  ../installer/InstallerResources.robot
Resource  ../GeneralTeardownResource.robot
Resource  ../mcs_router/McsRouterResources.robot

Default Tags  INSTALLER

*** Test Cases ***

#Get Fileset
#    [Tags]    DEBUG  INSTALLER  TESTFAILURE
#    Require Fresh Install
#
#    Check Expected Base Processes Are Running
#    ${DirectoryInfo}  ${FileInfo}  ${SymbolicLinkInfo} =   get_file_info_for_installation_debug
#
#    ## Check Directory Structure
#    Log  ${DirectoryInfo}
#    Log  ${FileInfo}
#    Log  ${SymbolicLinkInfo}

Verify that the full installer works correctly
## -------------------------------------READ----ME------------------------------------------------------
## Please note that these tests rely on the files in InstallSet being upto date. To regenerate these run
## an install manually and run the generateFromInstallDir.sh from InstallSet directory.
## WARNING
## If you generate this from a local build please make sure that you have blatted the distribution
## folder before remaking it. Otherwise old content can slip through to new builds and corrupt the
## fileset.
##
## ALTERNATIVELY ##
## Run the commented out *test* above ("Get Fileset"). Open the report and copy the logged "DirectoryInfo",
## "FileInfo", and "SymbolicLinkInfo" into their respective files in the installset and use a find and
## replace with regex enabled to swap \|\| for \n
## WARNING:
## ENSURE THAT THE CHANGES YOU SEE IN THE COMMIT DIFF ARE WHAT YOU WANT
## -----------------------------------------------------------------------------------------------------
    [Tags]    DEBUG  INSTALLER  SMOKE  TAP_TESTS
    [Teardown]  Install Tests Teardown With Installed File Replacement
    Require Fresh Install
    Check Expected Base Processes Are Running
    Unmount All Comms Component Folders


#    ${r} =  Run Process  systemctl  stop  sophos-spl
#    Should Be Equal As Strings  ${r.rc}  0
    ${DirectoryInfo}  ${FileInfo}  ${SymbolicLinkInfo} =   get file info for installation
    Set Test Variable  ${FileInfo}
    Set Test Variable  ${DirectoryInfo}
    Set Test Variable  ${SymbolicLinkInfo}
    ## Check Directory Structure
    Log  ${DirectoryInfo}
    ${ExpectedDirectoryInfo}=  Get File  ${ROBOT_TESTS_DIR}/installer/InstallSet/DirectoryInfo
    Should Be Equal As Strings  ${ExpectedDirectoryInfo}  ${DirectoryInfo}

    ## Check File Info
    # wait for /opt/sophos-spl/base/mcs/status/cache/ALC.xml to exist
    ${ExpectedFileInfo}=  Get File  ${ROBOT_TESTS_DIR}/installer/InstallSet/FileInfo
    Should Be Equal As Strings  ${ExpectedFileInfo}  ${FileInfo}

    ## Check Symbolic Links
    ${ExpectedSymbolicLinkInfo} =  Get File  ${ROBOT_TESTS_DIR}/installer/InstallSet/SymbolicLinkInfo
    Should Be Equal As Strings  ${ExpectedSymbolicLinkInfo}  ${SymbolicLinkInfo}

    ## Check systemd files
    ${SystemdInfo}=  get systemd file info
    ${ExpectedSystemdInfo}=  Get File  ${ROBOT_TESTS_DIR}/installer/InstallSet/SystemdInfo
    Should Be Equal As Strings  ${ExpectedSystemdInfo}  ${SystemdInfo}

Verify Sockets Have Correct Permissions
    [Tags]    DEBUG  INSTALLER
    Require Fresh Install

    Check Expected Base Processes Are Running
    Unmount All Comms Component Folders

    ${ActualDictOfSockets} =    Get Dictionary Of Actual Sockets And Permissions
    ${ExpectedDictOfSockets} =  Get Dictionary Of Expected Sockets And Permissions

    Dictionaries Should Be Equal  ${ActualDictOfSockets}  ${ExpectedDictOfSockets}

Verify Base Processes Have Correct Permissions
    Require Fresh Install
    Check Expected Base Processes Are Running
    Check owner of process   sophos_managementagent   sophos-spl-user    sophos-spl-group
    Check owner of process   UpdateScheduler   sophos-spl-updatescheduler   sophos-spl-group
    Check owner of process   tscheduler   sophos-spl-user    sophos-spl-group
    Check owner of process   sophos_watchdog   root  root
    ${watchdog_pid}=     Run Process    pgrep    -f  sophos_watchdog

    ${FirstCommspid} =     Run Process    pgrep    -P  ${watchdog_pid.stdout}  -f      CommsComponent
    log   ${FirstCommspid.stdout}
    ${result} =     Run Process    ps    -o    user   -p   ${FirstCommspid.stdout}
    Should Contain  ${result.stdout}   sophos-spl-local
    ${result} =     Run Process    ps    -o    group   -p   ${FirstCommspid.stdout}
    Should Contain  ${result.stdout}   sophos-spl-group

    ${SecondCommspid} =     Run Process    pgrep    -P  ${FirstCommspid.stdout}  -f      CommsComponent
    log   ${SecondCommspid.stdout}
    ${result} =     Run Process    ps    -o    user   -p   ${SecondCommspid.stdout}
    Should Contain  ${result.stdout}   sophos-spl-network
    ${result} =     Run Process    ps    -o    group   -p   ${SecondCommspid.stdout}
    Should Contain  ${result.stdout}   sophos-spl-network

Verify MCS Folders Have Correct Permissions
    [Tags]    DEBUG  INSTALLER
    Require Fresh Install

    Check Expected Base Processes Are Running
    Unmount All Comms Component Folders

    ${ActualDictOfSockets} =    Get Dictionary Of Actual Mcs Folders And Permissions
    ${ExpectedDictOfSockets} =  Get Directory Of Expected Mcs Folders And Permissions

    Dictionaries Should Be Equal  ${ActualDictOfSockets}  ${ExpectedDictOfSockets}

Verify Base Logs Have Correct Permissions
    [Tags]    DEBUG  INSTALLER
    Require Fresh Install

    Check Expected Base Processes Are Running
    Unmount All Comms Component Folders    

    ${ActualDictOfLogs} =    Get Dictionary Of Actual Base Logs And Permissions
    ${ExpectedDictOfLogs} =  Get Dictionary Of Expected Base Logs And Permissions

    Dictionaries Should Be Equal  ${ActualDictOfLogs}  ${ExpectedDictOfLogs}

Verify that uninstall works correctly
    [Tags]   DEBUG  INSTALLER  SMOKE
    Ensure Uninstalled
    Run Full Installer
    Run Process    ${SOPHOS_INSTALL}/bin/uninstall.sh  --force
    Verify Sophos Users And Groups are Removed
    Should Not Exist   ${SOPHOS_INSTALL}

Verify Machine Id is created correctly
    Ensure Uninstalled
    Require Fresh Install
    ${machineIdFromPython}  FullInstallerUtils.Get Machine Id Generate By Python
    ${machineID}            Get File   ${SOPHOS_INSTALL}/base/etc/machine_id.txt
    Should be Equal    ${machineIdFromPython}     ${machineID}    Machine id differ from the value generated by python (${machineIdFromPython})

Verify Saved Environment Proxy File is created correctly
    Ensure Uninstalled
    ${HttpsProxyAddress}=  Set Variable  https://username:password@dummyproxy.com
    Set Environment Variable  https_proxy  ${HttpsProxyAddress}
    Require Fresh Install
    File Should Exist  ${SOPHOS_INSTALL}/base/etc/savedproxy.config
    ${SavedProxyFileContents}=  Get File  ${SOPHOS_INSTALL}/base/etc/savedproxy.config
    Should Contain  ${SavedProxyFileContents}  ${HttpsProxyAddress}


Verify repeat installation doesnt change permissions
    [Tags]   INSTALLER
    Ensure Uninstalled
    Should Not Exist   ${SOPHOS_INSTALL}
    Run Full Installer Expecting Code  0
    Should Exist   ${SOPHOS_INSTALL}
    Unmount All Comms Component Folders
    ${DirectoryInfo}=  Run Process  find  ${SOPHOS_INSTALL}  -type  d  -exec  stat  -c  %a, %G, %U, %n  {}  +
    Create File    ./tmp/NewDirInfo  ${DirectoryInfo.stdout}
    ${DirectoryInfo}=  Run Process  sort  ./tmp/NewDirInfo
    log  ${DirectoryInfo.stdout}

    Run Full Installer Expecting Code  0
    Should Exist   ${SOPHOS_INSTALL}
    Unmount All Comms Component Folders    
    ${DirectoryInfo2}=  Run Process  find  ${SOPHOS_INSTALL}  -type  d  -exec  stat  -c  %a, %G, %U, %n  {}  +
    Create File    ./tmp/NewDirInfo  ${DirectoryInfo2.stdout}
    ${DirectoryInfo2}=  Run Process  sort  ./tmp/NewDirInfo
    Log  ${DirectoryInfo2.stdout}

    Should Be Equal As Strings  ${DirectoryInfo.stdout}  ${DirectoryInfo2.stdout}

Verify VERSION File Is Installed And Contains Sensible Details
    [Tags]   INSTALLER
    Require Fresh Install
    File Should Exist  ${SOPHOS_INSTALL}/base/VERSION.ini
    VERSION Ini File Contains Proper Format For Product Name  ${SOPHOS_INSTALL}/base/VERSION.ini   Sophos Server Protection Linux - Base Component

Installer Fails If The System Has A Libc Version Below That Of The Build Machine
    [Teardown]  Fake Ldd Teardown
    Ensure Uninstalled
    Should Not Exist   ${SOPHOS_INSTALL}
    Set Test Variable  ${LowGlibcVersion}  1.0
    Create Fake Ldd Executable With Version As Argument And Add It To Path  ${LowGlibcVersion}
    ${SystemGlibcVersion} =  Get Glibc Version From System
    Should Be Equal As Strings  ${SystemGlibcVersion}  ${LowGlibcVersion}
    ${consoleOutput} =  Run Full Installer Expecting Code  50
    Validate Incorrect Glibc Console Output  ${consoleOutput}

Installer Succeeds If The System Has A Libc Version Equal To That Of The Build Machine
    [Teardown]  Fake Ldd Teardown
    Ensure Uninstalled
    Should Not Exist   ${SOPHOS_INSTALL}
    ${buildMachineGlibcVersion} =  Get Glibc Version From Full Installer
    Create Fake Ldd Executable With Version As Argument And Add It To Path  ${buildMachineGlibcVersion}
    ${SystemGlibcVersion} =  Get Glibc Version From System
    Should Be Equal As Strings  ${SystemGlibcVersion}  ${buildMachineGlibcVersion}
    ${consoleOutput} =  Run Full Installer Expecting Code  0

Installer Succeeds If The System Has A Libc Version Greater Than That Of The Build Machine
    [Teardown]  Fake Ldd Teardown
    Ensure Uninstalled
    Should Not Exist   ${SOPHOS_INSTALL}
    Set Test Variable  ${HighGlibcVersion}  999.999
    Create Fake Ldd Executable With Version As Argument And Add It To Path  ${HighGlibcVersion}
    ${SystemGlibcVersion} =  Get Glibc Version From System
    Should Be Equal As Strings  ${SystemGlibcVersion}  ${HighGlibcVersion}
    ${consoleOutput} =  Run Full Installer Expecting Code  0

Test Successfull Install Does Not Contain Error And Noise
    [Tags]  TAP_TESTS  INSTALLER
    Ensure Uninstalled
    Should Not Exist   ${SOPHOS_INSTALL}
    ${consoleOutput} =  Run Full Installer Without X Set
    Should Be Equal As Strings  ${consoleOutput}  Installation complete, performing post install steps\n

Version Copy Correctly Set Links
    [Teardown]  VersionCopy Teardown
    Require Installed
    ${VersionCopy} =  Set Variable  ${SOPHOS_INSTALL}/base/bin/versionedcopy
    File Should Exist   ${VersionCopy}
    Create Directory  /tmp/testVersionCopy/files
    Create File    /tmp/testVersionCopy/files/liba.so.1.2.3  123
    Create File    /tmp/testVersionCopy/files/liba.so.1.2.4  124
    Run Process  ${VersionCopy} files/liba.so.1.2.3  shell=True  cwd=/tmp/testVersionCopy  env:SOPHOS_INSTALL=/tmp/target
    ${lib123} =  Get File  /tmp/target/liba.so
    Should Be Equal  ${lib123}  123

    Run Process  ${VersionCopy} files/liba.so.1.2.4  shell=True  cwd=/tmp/testVersionCopy  env:SOPHOS_INSTALL=/tmp/target
    ${lib124} =  Get File  /tmp/target/liba.so
    Should Be Equal  ${lib124}  124

  
    Run Process  ${VersionCopy} files/liba.so.1.2.3  shell=True  cwd=/tmp/testVersionCopy  env:SOPHOS_INSTALL=/tmp/target
    ${lib123} =  Get File  /tmp/target/liba.so
    Should Be Equal  ${lib123}  123

Installer Resets Bad Permissions On Current Proxy file
    Require Installed
    Run Process   systemctl stop sophos-spl   shell=yes
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  0.5 secs
    ...  Check Watchdog Not Running
    Create File  ${SOPHOS_INSTALL}/base/etc/sophosspl/current_proxy  {}
    ${result} =  Run Process  chown root:root ${SOPHOS_INSTALL}/base/etc/sophosspl/current_proxy    shell=True
    Should Be Equal As Integers    ${result.rc}   0  Failed to set permission to file. Reason: ${result.stderr}
    Run Full Installer
    Check Current Proxy Is Created With Correct Content And Permissions  {}

Installer Copies Install Options File
    Ensure Uninstalled
    Should Not Exist   ${SOPHOS_INSTALL}
    Create File  /tmp/InstallOptionsTestFile  --thing
    Set Environment Variable  INSTALL_OPTIONS_FILE  /tmp/InstallOptionsTestFile
    Run Full Installer Expecting Code  0
    Should Exist   ${SOPHOS_INSTALL}
    Should Exist   ${ETC_DIR}/install_options
    ${contents} =  Get File  ${ETC_DIR}/install_options
    Should Contain  ${contents}  --thing

*** Keywords ***

Check Installer Running
    ${result} =    Run Process  pgrep  -f  install.sh
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Integers    ${result.rc}    0   install.sh not running

Check Installer Not Running
    ${result} =    Run Process  pgrep  -f  install.sh
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Not Be Equal As Integers    ${result.rc}   0   install.sh still running

Save Current InstalledFiles To Local Path
    Create File  ${ROBOT_TESTS_DIR}/installer/InstallSet/FileInfo  ${FileInfo}
    Create File  ${ROBOT_TESTS_DIR}/installer/InstallSet/DirectoryInfo  ${DirectoryInfo}
    Create File  ${ROBOT_TESTS_DIR}/installer/InstallSet/SymbolicLinkInfo  ${SymbolicLinkInfo}

VersionCopy Teardown
    Run Keyword If Test Failed   Display List Files Dash L In Directory   /tmp/testVersionCopy/files
    Run Keyword If Test Failed   Display List Files Dash L In Directory   /tmp/target
    Remove Directory  /tmp/target    recursive=True
    Remove Directory  /tmp/testVersionCopy  recursive=True

Install Tests Teardown
    General Test Teardown
    Remove Environment Variable  https_proxy
    Run Keyword If Test Failed  Dump All Processes
    Ensure Uninstalled
    Run Keyword And Ignore Error  Remove File  /tmp/InstallOptionsTestFile

Install Tests Teardown With Installed File Replacement
    Run Keyword If Test Failed  Save Current InstalledFiles To Local Path
    Install Tests Teardown

Local Suite Setup
    ${ORIGINAL_PATH} =  Get Environment Variable  PATH
    Set Suite Variable  ${ORIGINAL_PATH}  ${ORIGINAL_PATH}

Fake Ldd Teardown
    Install Tests Teardown
    Clean Up Fake Ldd Executable

Check owner of process
    [Arguments]  ${process_name}  ${user_name}  ${group_name}
    ${pid} =     Run Process    pgrep    -f      ${process_name}
    log   ${pid.stdout}
    ${result} =     Run Process    ps    -o    user   -p   ${pid.stdout}
    Should Contain  ${result.stdout}   ${user_name}
    ${result} =     Run Process    ps    -o    group   -p   ${pid.stdout}
    Should Contain  ${result.stdout}   ${group_name}

Create Fake Ldd Executable With Version As Argument And Add It To Path
    [Arguments]  ${version}
    ${FakeLddDirectory} =  Add Temporary Directory  FakeExecutable
    ${script} =     Catenate    SEPARATOR=\n
    ...    \#!/bin/bash
    ...    echo "ldd (Ubuntu GLIBC 99999-ubuntu1) ${version}"
    ...    \
    Create File    ${FakeLddDirectory}/ldd   content=${script}
    Run Process    chmod  +x  ${FakeLddDirectory}/ldd

    ${PATH} =  Get Environment Variable  PATH
    ${result} =  Run Process  ${FakeLddDirectory}/ldd
    Log  ${result.stdout}

    Set Environment Variable  PATH  ${FakeLddDirectory}:${PATH}

Clean Up Fake Ldd Executable
    Set Environment Variable  PATH  ${ORIGINAL_PATH}
    Cleanup Temporary Folders

Get Glibc Version From System
    ${result} =  Run Process  ldd --version | grep 'ldd (.*)' | rev | cut -d ' ' -f 1 | rev  shell=True
    Should Be Equal As Strings  ${result.rc}  0
    [Return]  ${result.stdout}

Verify Sophos Users And Groups are Removed
    Verify Group Removed  sophos-spl-group
    Verify User Removed   sophos-spl-user

    Verify User Removed   sophos-spl-network
    Verify User Removed   sophos-spl-local
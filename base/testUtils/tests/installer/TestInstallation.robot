*** Settings ***
Test Teardown    Install Tests Teardown

Suite Setup  Local Suite Setup

Documentation    Test Installation

Library    ${COMMON_TEST_LIBS}/FullInstallerUtils.py
Library    ${COMMON_TEST_LIBS}/LogUtils.py
Library    ${COMMON_TEST_LIBS}/OSUtils.py
Library    ${COMMON_TEST_LIBS}/TemporaryDirectoryManager.py
Library    Collections

Resource  ${COMMON_TEST_ROBOT}/GeneralTeardownResource.robot
Resource  ${COMMON_TEST_ROBOT}/InstallerResources.robot
Resource  ${COMMON_TEST_ROBOT}/McsRouterResources.robot
Resource  ${COMMON_TEST_ROBOT}/ThinInstallerResources.robot
Resource  ${COMMON_TEST_ROBOT}/WatchdogResources.robot

Force Tags  INSTALLER  TAP_PARALLEL2

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
    [Tags]    DEBUG  INSTALLER  SMOKE  TAP_PARALLEL2  BREAKS_DEBUG
    [Teardown]  Install Tests Teardown With Installed File Replacement
    Require Fresh Install
    Check Expected Base Processes Are Running

#    ${r} =  Run Process  systemctl  stop  sophos-spl
#    Should Be Equal As Strings  ${r.rc}  0
    ${DirectoryInfo}  ${FileInfo}  ${SymbolicLinkInfo} =   get file info for installation
    Set Test Variable  ${FileInfo}
    Set Test Variable  ${DirectoryInfo}
    Set Test Variable  ${SymbolicLinkInfo}
    ## Check Directory Structure
    Log  ${DirectoryInfo}
    ${ExpectedDirectoryInfo}=  Get File  ${ROBOT_TESTS_DIR}/tests/installer/InstallSet/DirectoryInfo
    Should Be Equal As Strings  ${ExpectedDirectoryInfo}  ${DirectoryInfo}

    ## Check File Info
    # wait for /opt/sophos-spl/base/mcs/status/cache/ALC.xml to exist
    ${ExpectedFileInfo}=  Get File  ${ROBOT_TESTS_DIR}/tests/installer/InstallSet/FileInfo
    ${ExpectedFileInfo} =    Adjust Base Install Set Expected File Info For Platform    ${ExpectedFileInfo}
    Should Be Equal As Strings  ${ExpectedFileInfo}  ${FileInfo}

    ## Check Symbolic Links
    ${ExpectedSymbolicLinkInfo} =  Get File  ${ROBOT_TESTS_DIR}/tests/installer/InstallSet/SymbolicLinkInfo
    ${ExpectedSymbolicLinkInfo} =    Adjust Base Install Set Expected Symbolic Link Info For Platform    ${ExpectedSymbolicLinkInfo}
    Should Be Equal As Strings  ${ExpectedSymbolicLinkInfo}  ${SymbolicLinkInfo}

    ## Check systemd files
    ${SystemdInfo}=  get systemd file info
    ${ExpectedSystemdInfo}  Run Keyword If  not os.path.isdir("/lib/systemd/system/")  Get File   ${ROBOT_TESTS_DIR}/tests/installer/InstallSet/SystemdInfo_withoutLibSystemd
    ...  ELSE IF    not os.path.isdir("/usr/lib/systemd/system/")  Get File   ${ROBOT_TESTS_DIR}/tests/installer/InstallSet/SystemdInfo_withoutUsrLibSystemd
    ...  ELSE  Get File   ${ROBOT_TESTS_DIR}/tests/installer/InstallSet/SystemdInfo
    Should Be Equal As Strings  ${ExpectedSystemdInfo}  ${SystemdInfo}

Verify Sockets Have Correct Permissions
    [Tags]    DEBUG  INSTALLER
    Require Fresh Install

    Check Expected Base Processes Are Running

    ${ActualDictOfSockets} =    Get Dictionary Of Actual Sockets And Permissions
    ${ExpectedDictOfSockets} =  Get Dictionary Of Expected Sockets And Permissions

    Dictionaries Should Be Equal  ${ActualDictOfSockets}  ${ExpectedDictOfSockets}

Running installer without /usr/sbin in PATH works
    run_full_installer_with_truncated_path
    Check Expected Base Processes Are Running

Verify Base Processes Have Correct Permissions
    Require Fresh Install
    Check Expected Base Processes Are Running
    Check owner of process   sophos_managementagent   sophos-spl-user    sophos-spl-group
    Check owner of process   sdu   sophos-spl-user    sophos-spl-group
    Check owner of process   UpdateScheduler   sophos-spl-updatescheduler   sophos-spl-group
    Check owner of process   tscheduler   sophos-spl-user    sophos-spl-group
    Check owner of process   sophos_watchdog   root  root
    ${watchdog_pid}=     Run Process    pgrep    -f  sophos_watchdog


Verify MCS Folders Have Correct Permissions
    [Tags]    DEBUG  INSTALLER
    Require Fresh Install

    Check Expected Base Processes Are Running

    ${ActualDictOfSockets} =    Get Dictionary Of Actual Mcs Folders And Permissions
    ${ExpectedDictOfSockets} =  Get Directory Of Expected Mcs Folders And Permissions

    Dictionaries Should Be Equal  ${ActualDictOfSockets}  ${ExpectedDictOfSockets}

Verify Base Logs Have Correct Permissions
    [Tags]    DEBUG  INSTALLER
    Require Fresh Install

    Check Expected Base Processes Are Running

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
    # Exclude on SLES12 until LINUXDAR-7094 is fixed
    [Tags]    EXCLUDE_SLES12
    Ensure Uninstalled
    Require Fresh Install
    ${machineIdFromPython}  FullInstallerUtils.Get Machine Id Generate By Python
    ${machineID}            Get File   ${SOPHOS_INSTALL}/base/etc/machine_id.txt
    Should be Equal    ${machineIdFromPython}     ${machineID}    Machine id differ from the value generated by python (${machineIdFromPython})

Verify Saved Environment Proxy File Is Created Correctly
    Ensure Uninstalled
    ${HttpsProxyAddress}=  Set Variable  https://username:password@dummyproxy.com
    Set Environment Variable  https_proxy  ${HttpsProxyAddress}
    Require Fresh Install
    File Should Exist  ${SOPHOS_INSTALL}/base/etc/savedproxy.config
    ${SavedProxyFileContents}=  Get File  ${SOPHOS_INSTALL}/base/etc/savedproxy.config
    Should Be Equal  ${SavedProxyFileContents}  ${HttpsProxyAddress}

Verify Saved Environment Proxy File Is Created Correctly Uppercase
    Ensure Uninstalled
    ${HttpsProxyAddress}=  Set Variable  https://username:password@dummyproxy.com
    Set Environment Variable  HTTPS_PROXY  ${HttpsProxyAddress}
    Require Fresh Install
    File Should Exist  ${SOPHOS_INSTALL}/base/etc/savedproxy.config
    ${SavedProxyFileContents}=  Get File  ${SOPHOS_INSTALL}/base/etc/savedproxy.config
    Should Be Equal  ${SavedProxyFileContents}  ${HttpsProxyAddress}


Verify repeat installation doesnt change permissions
    [Tags]   INSTALLER
    Ensure Uninstalled
    Should Not Exist   ${SOPHOS_INSTALL}
    Run Full Installer Expecting Code  0
    Should Exist   ${SOPHOS_INSTALL}
    ${DirectoryInfo}=  Run Process  find  ${SOPHOS_INSTALL}  -type  d  -exec  stat  -c  %a, %G, %U, %n  {}  +
    Create File    ./tmp/NewDirInfo  ${DirectoryInfo.stdout}
    ${DirectoryInfo}=  Run Process  sort  ./tmp/NewDirInfo
    log  ${DirectoryInfo.stdout}

    Run Full Installer Expecting Code  0
    Should Exist   ${SOPHOS_INSTALL}
    ${DirectoryInfo2}=  Run Process  find  ${SOPHOS_INSTALL}  -type  d  -exec  stat  -c  %a, %G, %U, %n  {}  +
    Create File    ./tmp/NewDirInfo  ${DirectoryInfo2.stdout}
    ${DirectoryInfo2}=  Run Process  sort  ./tmp/NewDirInfo
    Log  ${DirectoryInfo2.stdout}

    Should Be Equal As Strings  ${DirectoryInfo.stdout}  ${DirectoryInfo2.stdout}

Verify VERSION File Is Installed And Contains Sensible Details
    [Tags]   INSTALLER
    Require Fresh Install
    File Should Exist  ${SOPHOS_INSTALL}/base/VERSION.ini
    VERSION Ini File Contains Proper Format For Product Name  ${SOPHOS_INSTALL}/base/VERSION.ini   SPL-Base-Component

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

Version Copy Copies Files When Group Contains Long List Of Users
    [Teardown]  Teardown Clean Up Group File With Large Group Creation
    Require Installed
    Setup Group File With Large Group Creation

    ${VersionCopy} =  Set Variable  ${SOPHOS_INSTALL}/base/bin/versionedcopy
    File Should Exist   ${VersionCopy}
    Create Directory  /tmp/testVersionCopy/files
    Create File    /tmp/testVersionCopy/files/liba.so.1.2.3  123
    Run Process  ${VersionCopy} files/liba.so.1.2.3  shell=True  cwd=/tmp/testVersionCopy  env:SOPHOS_INSTALL=/tmp/target
    ${lib123} =  Get File  /tmp/target/liba.so
    Should Be Equal  ${lib123}  123

    Create File    /tmp/testVersionCopy/files/liba.so.1.2.3  12345
    ${result} =  Run Process  ${VersionCopy} files/liba.so.1.2.3  shell=True  cwd=/tmp/testVersionCopy  env:SOPHOS_INSTALL=/tmp/target
    log  ${result.stdout}
    log  ${result.stderr}
    ${lib123} =  Get File  /tmp/target/liba.so
    Should Be Equal  ${lib123}  12345

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

Installer Resets Ownership Of Stale MCS Router Process ID File
    [Tags]    DEBUG  INSTALLER
    Require Fresh Install
    Check Expected Base Processes Are Running
    ${mcs_router_pid_file} =    Set Variable    ${SOPHOS_INSTALL}/var/lock-sophosspl/mcsrouter.pid
    Wait Until Created  ${mcs_router_pid_file}
    Wdctl Stop Plugin   mcsrouter
    create file  ${mcs_router_pid_file}
    Run Process  chown  sophos-spl-user:sophos-spl-group  ${mcs_router_pid_file}
    Run Process  chmod  600  ${mcs_router_pid_file}

    # Make sure our chmod and chown worked to break the permissions.
    File Exists With Permissions  ${mcs_router_pid_file}  sophos-spl-user  sophos-spl-group  -rw-------
    Run Full Installer
    Wait Until Created    ${mcs_router_pid_file}

    # PID file should be fixed up to have right permissions
    File Exists With Permissions  ${mcs_router_pid_file}  sophos-spl-local  sophos-spl-group  -rw-------

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

Installer Copies MCS Config Files Into place When Passed In As Args
    Ensure Uninstalled
    Should Not Exist   ${SOPHOS_INSTALL}
    Create File  /tmp/mcsconfig  content="MCSID=root\nMCSToken=root"
    Create File  /tmp/policyconfig  content="MCSID=policy\nMCSToken=policy"
    Run Full Installer Expecting Code  0  --mcs-config  /tmp/mcsconfig  --mcs-policy-config  /tmp/policyconfig
    File Exists With Permissions  ${SOPHOS_INSTALL}/base/etc/mcs.config  sophos-spl-local  sophos-spl-group  -rw-r-----
    ${root_contents} =  Get File  ${SOPHOS_INSTALL}/base/etc/mcs.config
    Should Be Equal As Strings  ${root_contents}  "MCSID=root\nMCSToken=root"
    File Exists With Permissions  ${MCS_CONFIG}  sophos-spl-local  sophos-spl-group  -rw-r-----
    ${policy_contents} =  Get File  ${MCS_CONFIG}
    Should Be Equal As Strings  ${policy_contents}  "MCSID=policy\nMCSToken=policy"

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
    Create File  ${ROBOT_TESTS_DIR}/tests/installer/InstallSet/FileInfo  ${FileInfo}
    Create File  ${ROBOT_TESTS_DIR}/tests/installer/InstallSet/DirectoryInfo  ${DirectoryInfo}
    Create File  ${ROBOT_TESTS_DIR}/tests/installer/InstallSet/SymbolicLinkInfo  ${SymbolicLinkInfo}

VersionCopy Teardown
    Run Keyword If Test Failed   Display List Files Dash L In Directory   /tmp/testVersionCopy/files
    Run Keyword If Test Failed   Display List Files Dash L In Directory   /tmp/target
    Remove Directory  /tmp/target    recursive=True
    Remove Directory  /tmp/testVersionCopy  recursive=True

Install Tests Teardown
    General Test Teardown
    Remove Environment Variable  https_proxy
    Remove Environment Variable  HTTPS_PROXY
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
    [Arguments]  ${process_name}  ${expected_user_name}  ${expected_group_name}
    ${pid} =     Run Process    pgrep    -f      ${process_name}
    log   ${pid.stdout}
    ${user_name} =  Get Process Owner  ${pid.stdout}
    Should Contain  ${user_name}   ${expected_user_name}
    ${group_name} =  Get Process Group  ${pid.stdout}
    Should Contain  ${group_name}   ${expected_group_name}

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

    Verify User Removed   sophos-spl-local


Teardown Clean Up Group File With Large Group Creation
    Teardown Group File With Large Group Creation
    VersionCopy Teardown

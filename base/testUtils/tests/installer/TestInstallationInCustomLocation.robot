*** Settings ***
Default Tags  INSTALLER    TAP_PARALLEL5    CUSTOM_INSTALL_PATH

Library    Collections
Library    OperatingSystem
Library    Process

Library    ../../libs/OSUtils.py
Library    ../../libs/FullInstallerUtils.py
Library    ../../libs/Watchdog.py

Resource    ../GeneralTeardownResource.robot

Suite Setup    Create Directory     /opt/etc
Suite Teardown    remove Directory    /opt/etc
Test Setup    Install Tests Setup With Custom Install Location
Test Teardown    Install Tests Teardown With Custom Install Location

*** Variables ***
${CUSTOM_INSTALL_DIRECTORY}    /opt/etc/sophos-spl
${INSTALL_SET_DIRECTORY}    ${ROBOT_TESTS_DIR}/installer/InstallSet


*** Test Cases ***
Verify Installation to Custom Location Sets Environment Correctly
    Require Fresh Install to Custom Location
    Check Expected Base Processes Are Running in Custom Location

    ${serviceDir} =  get_service_folder
    ${contents}=    Get File     ${serviceDir}/sophos-spl.service
    Should Contain    ${contents}    Environment="SOPHOS_INSTALL=${CUSTOM_INSTALL_DIRECTORY}"

Verify that the full installer works correctly in a custom location
    [Tags]    DEBUG  INSTALLER  SMOKE  TAP_PARALLEL5  BREAKS_DEBUG
    Require Fresh Install to Custom Location
    Check Expected Base Processes Are Running in Custom Location

    Set Local Variable    ${SOPHOS_INSTALL}    ${CUSTOM_INSTALL_DIRECTORY}
    ${DirectoryInfo}  ${FileInfo}  ${SymbolicLinkInfo} =   get file info for installation
    Set Test Variable  ${FileInfo}
    Set Test Variable  ${DirectoryInfo}
    Set Test Variable  ${SymbolicLinkInfo}
    ## Check Directory Structure
    Log  ${DirectoryInfo}
    ${ExpectedDirectoryInfo}=  file_info_with_custom_install_directory    ${INSTALL_SET_DIRECTORY}/DirectoryInfo    ${CUSTOM_INSTALL_DIRECTORY}
    Should Be Equal As Strings  ${ExpectedDirectoryInfo}  ${DirectoryInfo}

    ## Check File Info
    ${ExpectedFileInfo}=  file_info_with_custom_install_directory    ${INSTALL_SET_DIRECTORY}/FileInfo    ${CUSTOM_INSTALL_DIRECTORY}
    Should Be Equal As Strings  ${ExpectedFileInfo}  ${FileInfo}

    ## Check Symbolic Links
    ${ExpectedSymbolicLinkInfo} =  file_info_with_custom_install_directory    ${INSTALL_SET_DIRECTORY}/SymbolicLinkInfo    ${CUSTOM_INSTALL_DIRECTORY}
    Should Be Equal As Strings  ${ExpectedSymbolicLinkInfo}  ${SymbolicLinkInfo}

    ## Check systemd files
    ${SystemdInfo}=  get systemd file info
    ${ExpectedSystemdInfo}  Run Keyword If  not os.path.isdir("/lib/systemd/system/")  Get File   ${INSTALL_SET_DIRECTORY}/SystemdInfo_withoutLibSystemd
    ...  ELSE IF    not os.path.isdir("/usr/lib/systemd/system/")  Get File   ${INSTALL_SET_DIRECTORY}/SystemdInfo_withoutUsrLibSystemd
    ...  ELSE  Get File   ${INSTALL_SET_DIRECTORY}/SystemdInfo
    Should Be Equal As Strings  ${ExpectedSystemdInfo}  ${SystemdInfo}

Verify Sockets Have Correct Permissions
    [Tags]    DEBUG  INSTALLER
    Require Fresh Install to Custom Location

    Check Expected Base Processes Are Running in Custom Location

    ${ActualDictOfSockets} =    get_dictionary_of_actual_sockets_and_permissions    ${CUSTOM_INSTALL_DIRECTORY}
    ${ExpectedDictOfSockets} =  get_dictionary_of_expected_sockets_and_permissions    ${CUSTOM_INSTALL_DIRECTORY}

    Dictionaries Should Be Equal  ${ActualDictOfSockets}  ${ExpectedDictOfSockets}

Verify Base Processes Have Correct Permissions
    Require Fresh Install to Custom Location
    Check Expected Base Processes Are Running in Custom Location
    Check owner of process    sophos_managementagent   sophos-spl-user    sophos-spl-group
    Check owner of process    sdu   sophos-spl-user    sophos-spl-group
    Check owner of process    UpdateScheduler   sophos-spl-updatescheduler   sophos-spl-group
    Check owner of process    tscheduler   sophos-spl-user    sophos-spl-group
    Check owner of process    sophos_watchdog   root  root
    ${watchdog_pid}=     Run Process    pgrep    -f  sophos_watchdog


Verify MCS Folders Have Correct Permissions
    [Tags]    DEBUG  INSTALLER
    Require Fresh Install to Custom Location

    Check Expected Base Processes Are Running in Custom Location

    ${ActualDictOfSockets} =    get_dictionary_of_actual_mcs_folders_and_permissions    ${CUSTOM_INSTALL_DIRECTORY}
    ${ExpectedDictOfSockets} =  get_directory_of_expected_mcs_folders_and_permissions    ${CUSTOM_INSTALL_DIRECTORY}

    Dictionaries Should Be Equal  ${ActualDictOfSockets}  ${ExpectedDictOfSockets}

Verify Base Logs Have Correct Permissions
    [Tags]    DEBUG  INSTALLER
    Require Fresh Install to Custom Location

    Check Expected Base Processes Are Running in Custom Location

    ${ActualDictOfLogs} =    get_dictionary_of_actual_base_logs_and_permissions    ${CUSTOM_INSTALL_DIRECTORY}
    ${ExpectedDictOfLogs} =  get_dictionary_of_expected_base_logs_and_permissions    ${CUSTOM_INSTALL_DIRECTORY}

    Dictionaries Should Be Equal  ${ActualDictOfLogs}  ${ExpectedDictOfLogs}

Verify that uninstall works correctly
    [Tags]   DEBUG  INSTALLER  SMOKE
    run_full_installer
    Run Process    ${CUSTOM_INSTALL_DIRECTORY}/bin/uninstall.sh  --force
    Verify Group Removed  sophos-spl-group
    Verify User Removed   sophos-spl-user
    Verify User Removed   sophos-spl-local
    Should Not Exist   ${CUSTOM_INSTALL_DIRECTORY}

Verify Machine Id is created correctly
    # Exclude on SLES12 until LINUXDAR-7094 is fixed
    [Tags]    EXCLUDE_SLES12
    Require Fresh Install to Custom Location
    ${machineIdFromPython}  FullInstallerUtils.Get Machine Id Generate By Python
    ${machineID}            Get File   ${CUSTOM_INSTALL_DIRECTORY}/base/etc/machine_id.txt
    Should be Equal    ${machineIdFromPython}     ${machineID}    Machine id differ from the value generated by python (${machineIdFromPython})

Verify Saved Environment Proxy File Is Created Correctly
    ${HttpsProxyAddress}=  Set Variable  https://username:password@dummyproxy.com
    Set Environment Variable  https_proxy  ${HttpsProxyAddress}
    Require Fresh Install to Custom Location
    File Should Exist  ${CUSTOM_INSTALL_DIRECTORY}/base/etc/savedproxy.config
    ${SavedProxyFileContents}=  Get File  ${CUSTOM_INSTALL_DIRECTORY}/base/etc/savedproxy.config
    Should Be Equal  ${SavedProxyFileContents}  ${HttpsProxyAddress}


Installer Copies MCS Config Files Into place When Passed In As Args
    Should Not Exist   ${CUSTOM_INSTALL_DIRECTORY}
    Create File  /tmp/mcsconfig  content="MCSID=root\nMCSToken=root"
    Create File  /tmp/policyconfig  content="MCSID=policy\nMCSToken=policy"
    run_full_installer  --mcs-config  /tmp/mcsconfig  --mcs-policy-config  /tmp/policyconfig
    File Exists With Permissions  ${CUSTOM_INSTALL_DIRECTORY}/base/etc/mcs.config  sophos-spl-local  sophos-spl-group  -rw-r-----
    ${root_contents} =  Get File  ${CUSTOM_INSTALL_DIRECTORY}/base/etc/mcs.config
    Should Be Equal As Strings  ${root_contents}  "MCSID=root\nMCSToken=root"
    File Exists With Permissions  ${CUSTOM_INSTALL_DIRECTORY}/base/etc/sophosspl/mcs.config  sophos-spl-local  sophos-spl-group  -rw-r-----
    ${policy_contents} =  Get File  ${CUSTOM_INSTALL_DIRECTORY}/base/etc/sophosspl/mcs.config
    Should Be Equal As Strings  ${policy_contents}  "MCSID=policy\nMCSToken=policy"

*** Keywords ***
Install Tests Setup With Custom Install Location
    Uninstall_SSPL
    Set Environment Variable    SOPHOS_INSTALL    ${CUSTOM_INSTALL_DIRECTORY}

Install Tests Teardown With Custom Install Location
    Remove Environment Variable    SOPHOS_INSTALL    ${CUSTOM_INSTALL_DIRECTORY}
    Remove Environment Variable    https_proxy
    Remove Environment Variable    HTTPS_PROXY
    Run Keyword If Test Failed     dump_all_processes
    
    General Test Teardown    ${CUSTOM_INSTALL_DIRECTORY}
    Uninstall_SSPL
    Run Keyword And Ignore Error    Remove File    /tmp/InstallOptionsTestFile

Teardown Clean Up Group File With Large Group Creation
    Move File  /etc/group.bak  /etc/group
    ${content} =  Get File  /etc/group
    ${LongLine} =  Get File  ${SUPPORT_FILES}/misc/325CharEtcGroupLine
    Should Not Contain  ${content}  ${LongLine}
    VersionCopy Teardown

VersionCopy Teardown
    Run Keyword If Test Failed   Display List Files Dash L In Directory   /tmp/testVersionCopy/files
    Run Keyword If Test Failed   Display List Files Dash L In Directory   /tmp/target
    Remove Directory  /tmp/target    recursive=True
    Remove Directory  /tmp/testVersionCopy  recursive=True
    Install Tests Teardown With Custom Install Location

Require Fresh Install to Custom Location
    Uninstall_SSPL
    Should Not Exist   ${SOPHOS_INSTALL}
    Should Not Exist   ${CUSTOM_INSTALL_DIRECTORY}

    Kill Sophos Processes in Custom Location

    run_full_installer
    Should Exist   ${CUSTOM_INSTALL_DIRECTORY}
    Should Not Exist   ${SOPHOS_INSTALL}

    Verify User Created   sophos-spl-user
    Verify Group Created  sophos-spl-group
    Verify User Created   sophos-spl-local
    Verify User Created   sophos-spl-updatescheduler

    Should Exist   ${CUSTOM_INSTALL_DIRECTORY}/var/ipc
    Should Exist   ${CUSTOM_INSTALL_DIRECTORY}/logs/base
    Should Exist   ${CUSTOM_INSTALL_DIRECTORY}/tmp

Kill Sophos Processes in Custom Location
    ${result} =  Run Process   pgrep   sophos_watchdog
    Run Keyword If  ${result.rc} == 0  Run Process  kill  -9  ${result.stdout}
    ${result} =  Run Process   pgrep   -f    ${CUSTOM_INSTALL_DIRECTORY}/base/bin/sophos_managementagent
    Run Keyword If  ${result.rc} == 0  Run Process  kill  -9  ${result.stdout}
    ${result} =  Run Process   pgrep   mtr
    Run Keyword If  ${result.rc} == 0  Run Process  kill  -9  ${result.stdout}
    ${result} =  Run Process   pgrep   -f   ${CUSTOM_INSTALL_DIRECTORY}/base/bin/python3 -m mcsrouter
    Run Keyword If  ${result.rc} == 0  Run Process  kill  -9  ${result.stdout}
    ${result} =  Run Process   pgrep   UpdateScheduler
    Run Keyword If  ${result.rc} == 0  Run Process  kill  -9  ${result.stdout}
    ${result} =  Run Process   pgrep   tscheduler
    Run Keyword If  ${result.rc} == 0  Run Process  kill  -9  ${result.stdout}
    ${result} =  Run Process   pgrep   sdu
    Run Keyword If  ${result.rc} == 0  Run Process  kill  -9  ${result.stdout}
    ${result} =  Run Process   pgrep   liveresponse
    Run Keyword If  ${result.rc} == 0  Run Process  kill  -9  ${result.stdout}

Check Expected Base Processes Are Running in Custom Location
    ${result} =     Run Process     pgrep  -f   sophos_watchdog
    Should Be Equal As Integers     ${result.rc}    0
    ${result} =     Run Process     pgrep  -f   sophos_managementagent
    Should Be Equal As Integers     ${result.rc}    0
    ${result} =     Run Process     pgrep  -f   UpdateScheduler
    Should Be Equal As Integers     ${result.rc}    0
    ${result} =     Run Process     pgrep  -f   tscheduler
    Should Be Equal As Integers     ${result.rc}    0
    ${result} =     Run Process     pgrep  -f   sdu
    Should Be Equal As Integers     ${result.rc}    0
    verify_watchdog_config

Check owner of process
    [Arguments]  ${process_name}  ${expected_user_name}  ${expected_group_name}
    ${pid} =     Run Process    pgrep    -f      ${process_name}
    log   ${pid.stdout}
    ${user_name} =  Get Process Owner  ${pid.stdout}
    Should Contain  ${user_name}   ${expected_user_name}
    ${group_name} =  Get Process Group  ${pid.stdout}
    Should Contain  ${group_name}   ${expected_group_name}

Check Watchdog Not Running
    ${result} =    Run Process  pgrep  sophos_watchdog
    Should Not Be Equal As Integers    ${result.rc}    0
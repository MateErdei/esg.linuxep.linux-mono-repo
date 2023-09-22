*** Settings ***
Documentation    Suite description

Resource  ../installer/InstallerResources.robot
Resource  ResponseActionsResources.robot
Resource  ../watchdog/WatchdogResources.robot

Suite Setup     Require Fresh Install
Suite Teardown  Require Uninstalled

Test Teardown  Ra Teardown

Force Tags   RESPONSE_ACTIONS_PLUGIN  TAP_TESTS


*** Test Cases ***
Verify that the response actions installer works correctly
## -------------------------------------READ----ME------------------------------------------------------
## Please note that these tests rely on the files in InstallSet being upto date. To regenerate these run
## an install manually and run the generateFromInstallDir.sh from InstallSet directory.
## WARNING
## If you generate this from a local build please make sure that you have blatted the distribution
## folder before remaking it. Otherwise old content can slip through to new builds and corrupt the
## fileset.
## ENSURE THAT THE CHANGES YOU SEE IN THE COMMIT DIFF ARE WHAT YOU WANT
## -----------------------------------------------------------------------------------------------------
    # TODO LINUXDAR-7870: Remove coverage exclusion
    [Tags]   EXCLUDE_ON_COVERAGE
    [Teardown]  Response Actions Tests Teardown With Installed File Replacement
    Install Response Actions Directly
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check Log Contains  Logger responseactions configured for level: INFO  ${RESPONSE_ACTIONS_LOG_PATH}  response actions log
    ${DirectoryInfo}  ${FileInfo}  ${SymbolicLinkInfo} =   get file info for installation  responseactions
    Set Test Variable  ${FileInfo}
    Set Test Variable  ${DirectoryInfo}
    Set Test Variable  ${SymbolicLinkInfo}
    ## Check Directory Structure
    Log  ${DirectoryInfo}
    ${ExpectedDirectoryInfo}=  Get File  ${ROBOT_TESTS_DIR}/ra_plugin/InstallSet/DirectoryInfo
    Should Be Equal As Strings  ${ExpectedDirectoryInfo}  ${DirectoryInfo}

    ## Check File Info
    # wait for /opt/sophos-spl/base/mcs/status/cache/ALC.xml to exist
    ${ExpectedFileInfo}=  Get File  ${ROBOT_TESTS_DIR}/ra_plugin/InstallSet/FileInfo
    Should Be Equal As Strings  ${ExpectedFileInfo}  ${FileInfo}

    ## Check Symbolic Links
    ${ExpectedSymbolicLinkInfo} =  Get File  ${ROBOT_TESTS_DIR}/ra_plugin/InstallSet/SymbolicLinkInfo
    Should Be Equal As Strings  ${ExpectedSymbolicLinkInfo}  ${SymbolicLinkInfo}

Verify That Response Actions Logging Can Be Set Individually
    Install Response Actions Directly
    Create File         ${SOPHOS_INSTALL}/base/etc/logger.conf.local   [responseactions]\nVERBOSITY=DEBUG\n
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    Restart Response Actions
    wait_for_log_contains_from_mark  ${response_mark}  Logger responseactions configured for level: DEBUG

Uninstall Response actions cleans up
    [Teardown]  RA Uninstall Teardown
    Install Response Actions Directly
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check Log Contains  Logger responseactions configured for level: INFO  ${RESPONSE_ACTIONS_LOG_PATH}  response actions log
    Uninstall Response Actions
    Directory Should Not Exist  ${RESPONSE_ACTIONS_DIR}
    Check Response Actions Executable Not Running
*** Keywords ***
Response Actions Tests Teardown With Installed File Replacement
    Run Keyword If Test Failed  Save Current Response Actions InstalledFiles To Local Path
    RA Teardown

Save Current Response Actions InstalledFiles To Local Path
    Create File  ${ROBOT_TESTS_DIR}/ra_plugin/InstallSet/FileInfo  ${FileInfo}
    Create File  ${ROBOT_TESTS_DIR}/ra_plugin/InstallSet/DirectoryInfo  ${DirectoryInfo}
    Create File  ${ROBOT_TESTS_DIR}/ra_plugin/InstallSet/SymbolicLinkInfo  ${SymbolicLinkInfo}

RA Teardown
    Uninstall Response Actions
    Remove File     ${SOPHOS_INSTALL}/base/etc/logger.conf.local
    Create File     ${SOPHOS_INSTALL}/base/etc/logger.conf.local
    General Test Teardown

RA Uninstall Teardown
    Run Keyword If Test Failed   Uninstall Response Actions
    General Test Teardown
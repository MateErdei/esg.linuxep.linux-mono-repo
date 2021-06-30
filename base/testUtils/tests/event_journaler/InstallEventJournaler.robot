*** Settings ***
Documentation    Suite description

Resource  ../installer/InstallerResources.robot
Resource  EventJournalerResources.robot
Resource  ../watchdog/WatchdogResources.robot

Suite Setup     Require Fresh Install
Suite Teardown  Require Uninstalled

Test Teardown  General Test Teardown

Default Tags   EVENT_JOURNALER_PLUGIN


*** Test Cases ***
Verify that the event journaler installer works correctly
## -------------------------------------READ----ME------------------------------------------------------
## Please note that these tests rely on the files in InstallSet being upto date. To regenerate these run
## an install manually and run the generateFromInstallDir.sh from InstallSet directory.
## WARNING
## If you generate this from a local build please make sure that you have blatted the distribution
## folder before remaking it. Otherwise old content can slip through to new builds and corrupt the
## fileset.
## ENSURE THAT THE CHANGES YOU SEE IN THE COMMIT DIFF ARE WHAT YOU WANT
## -----------------------------------------------------------------------------------------------------
    [Teardown]  Event Journaler Tests Teardown With Installed File Replacement
    Install Event Journaler Directly

    ${DirectoryInfo}  ${FileInfo}  ${SymbolicLinkInfo} =   get file info for installation  eventjournaler
    Set Test Variable  ${FileInfo}
    Set Test Variable  ${DirectoryInfo}
    Set Test Variable  ${SymbolicLinkInfo}
    ## Check Directory Structure
    Log  ${DirectoryInfo}
    ${ExpectedDirectoryInfo}=  Get File  ${ROBOT_TESTS_DIR}/event_journaler/InstallSet/DirectoryInfo
    Should Be Equal As Strings  ${ExpectedDirectoryInfo}  ${DirectoryInfo}

    ## Check File Info
    # wait for /opt/sophos-spl/base/mcs/status/cache/ALC.xml to exist
    ${ExpectedFileInfo}=  Get File  ${ROBOT_TESTS_DIR}/event_journaler/InstallSet/FileInfo
    Should Be Equal As Strings  ${ExpectedFileInfo}  ${FileInfo}

    ## Check Symbolic Links
    ${ExpectedSymbolicLinkInfo} =  Get File  ${ROBOT_TESTS_DIR}/event_journaler/InstallSet/SymbolicLinkInfo
    Should Be Equal As Strings  ${ExpectedSymbolicLinkInfo}  ${SymbolicLinkInfo}

    ## Check systemd files
    ${SystemdInfo}=  get systemd file info
    ${ExpectedSystemdInfo}=  Get File  ${ROBOT_TESTS_DIR}/event_journaler/InstallSet/SystemdInfo
    Should Be Equal As Strings  ${ExpectedSystemdInfo}  ${SystemdInfo}

Verify That Live Response Logging Can Be Set Individually
    [Teardown]  Live Response Tests Teardown With Installed File Replacement
    Install Live Response Directly
    Create File         ${SOPHOS_INSTALL}/base/etc/logger.conf.local   [eventjournaler]\nVERBOSITY=DEBUG\n

    Restart Event Journaler
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Live Response Plugin Log Contains  Logger liveresponse configured for level: DEBUG


*** Keywords ***
Event Journaler Tests Teardown With Installed File Replacement
    Run Keyword If Test Failed  Save Current Event Journaler InstalledFiles To Local Path

Save Current Event Journaler InstalledFiles To Local Path
    Create File  ${ROBOT_TESTS_DIR}/event_journaler/InstallSet/FileInfo  ${FileInfo}
    Create File  ${ROBOT_TESTS_DIR}/event_journaler/InstallSet/DirectoryInfo  ${DirectoryInfo}
    Create File  ${ROBOT_TESTS_DIR}/event_journaler/InstallSet/SymbolicLinkInfo  ${SymbolicLinkInfo}
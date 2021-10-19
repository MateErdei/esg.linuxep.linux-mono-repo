*** Settings ***
Documentation   Suite description

Resource        EventJournalerResources.robot
Library         ../Libs/InstallerUtils.py

Suite Setup     Setup
Suite Teardown  Require Uninstalled

Test Teardown  Run Keywords
...            Event Journaler Teardown   AND
...            Uninstall Event Journaler


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
    Install Event Journaler Directly from SDDS

    ${DirectoryInfo}  ${FileInfo}  ${SymbolicLinkInfo} =   Get File Info For Installation
    Set Test Variable  ${FileInfo}
    Set Test Variable  ${DirectoryInfo}
    Set Test Variable  ${SymbolicLinkInfo}
    ## Check Directory Structure
    Log  ${DirectoryInfo}
    ${ExpectedDirectoryInfo}=  Get File  ${ROBOT_TESTS_DIR}/InstallSet/DirectoryInfo
    Should Be Equal As Strings  ${ExpectedDirectoryInfo}  ${DirectoryInfo}

    ## Check File Info
    # wait for /opt/sophos-spl/base/mcs/status/cache/ALC.xml to exist
    ${ExpectedFileInfo}=  Get File  ${ROBOT_TESTS_DIR}/InstallSet/FileInfo
    Should Be Equal As Strings  ${ExpectedFileInfo}  ${FileInfo}

    ## Check Symbolic Links
    ${ExpectedSymbolicLinkInfo} =  Get File  ${ROBOT_TESTS_DIR}/InstallSet/SymbolicLinkInfo
    Should Be Equal As Strings  ${ExpectedSymbolicLinkInfo}  ${SymbolicLinkInfo}

    ## Check systemd files
    ${SystemdInfo}=  get systemd file info
    ${ExpectedSystemdInfo}=  Get File  ${ROBOT_TESTS_DIR}/InstallSet/SystemdInfo
    Should Be Equal As Strings  ${ExpectedSystemdInfo}  ${SystemdInfo}

*** Keywords ***
Event Journaler Tests Teardown With Installed File Replacement
    Run Keyword If Test Failed  Save Current Event Journaler InstalledFiles To Local Path
    Event Journaler Teardown
    Uninstall Event Journaler

Save Current Event Journaler InstalledFiles To Local Path
    Create File  ${ROBOT_TESTS_DIR}/InstallSet/FileInfo  ${FileInfo}
    Create File  ${ROBOT_TESTS_DIR}/InstallSet/DirectoryInfo  ${DirectoryInfo}
    Create File  ${ROBOT_TESTS_DIR}/InstallSet/SymbolicLinkInfo  ${SymbolicLinkInfo}
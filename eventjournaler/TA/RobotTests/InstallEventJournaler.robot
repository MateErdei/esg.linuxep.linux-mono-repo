*** Settings ***
Documentation   Suite description

Resource        EventJournalerResources.robot
Library         ../Libs/InstallerUtils.py
Library         Collections

Suite Setup     Setup
Suite Teardown  Uninstall Base

Test Teardown  Run Keywords
...            Event Journaler Teardown   AND
...            Uninstall Event Journaler

Default Tags    TAP_PARALLEL3

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
    Wait Until Created    ${EVENT_JOURNALER_LOG_PATH}

    ${DirectoryInfo}  ${FileInfo}  ${SymbolicLinkInfo} =   Get File Info For Installation
    Set Test Variable  ${FileInfo}
    Set Test Variable  ${DirectoryInfo}
    Set Test Variable  ${SymbolicLinkInfo}
    ## Check Directory Structure
    Log  ${DirectoryInfo}
    ${ExpectedDirectoryInfo}=  Get File  ${ROBOT_SCRIPTS_PATH}/InstallSet/DirectoryInfo
    Should Be Equal As Strings  ${ExpectedDirectoryInfo}  ${DirectoryInfo}

    ## Check File Info
    # wait for /opt/sophos-spl/base/mcs/status/cache/ALC.xml to exist
    ${ExpectedFileInfo}=  Get File  ${ROBOT_SCRIPTS_PATH}/InstallSet/FileInfo
    Should Be Equal As Strings  ${ExpectedFileInfo}  ${FileInfo}

    ## Check Symbolic Links
    ${ExpectedSymbolicLinkInfo} =  Get File  ${ROBOT_SCRIPTS_PATH}/InstallSet/SymbolicLinkInfo
    Should Be Equal As Strings  ${ExpectedSymbolicLinkInfo}  ${SymbolicLinkInfo}

Check For Insecure Permissions
    Install Event Journaler Directly from SDDS
    # find all executables that are writable by sophos-spl-user or sophos-spl-group
    ${rc}   ${output} =    Run And Return Rc And Output
    ...     find ${SOPHOS_INSTALL} -type f -perm -0100 -\\( -\\( -user sophos-spl-user -perm -0200 -\\) -o -\\( -group sophos-spl-group -perm -0020 -\\) -o -perm -0002 -\\) -ls
    Should Be Equal As Integers  ${rc}  ${0}
    ${output} =   Replace String   ${output}   ${COMPONENT_ROOT_PATH}/   ${EMPTY}
    @{items} =    Split To Lines   ${output}
    Sort List   ${items}
    Log List   ${items}
    Should Be Empty   ${items}
    # find all executables in directories that are writable by sophos-spl-user or sophos-spl-group
    ${rc}   ${output} =    Run And Return Rc And Output
    ...     find ${SOPHOS_INSTALL} -type d -\\( -\\( -user sophos-spl-user -perm -0200 -\\) -o -\\( -group sophos-spl-group -perm -0020 -\\) -o -perm -0002 -\\) -exec find {} -maxdepth 1 -type f -\\( -perm -0100 -o -name '*.so*' -! -name '*.so.cache' -\\) \\;
    Should Be Equal As Integers  ${rc}  ${0}
    ${output} =   Replace String   ${output}   ${COMPONENT_ROOT_PATH}/   ${EMPTY}
    @{items} =    Split To Lines   ${output}
    Sort List   ${items}
    Log List   ${items}
    Should Be Empty   ${items}

*** Keywords ***
Event Journaler Tests Teardown With Installed File Replacement
    Run Keyword If Test Failed  Save Current Event Journaler InstalledFiles To Local Path
    Event Journaler Teardown
    Uninstall Event Journaler

Save Current Event Journaler InstalledFiles To Local Path
    Create File  ${ROBOT_SCRIPTS_PATH}/InstallSet/FileInfo  ${FileInfo}
    Create File  ${ROBOT_SCRIPTS_PATH}/InstallSet/DirectoryInfo  ${DirectoryInfo}
    Create File  ${ROBOT_SCRIPTS_PATH}/InstallSet/SymbolicLinkInfo  ${SymbolicLinkInfo}

*** Settings ***

Test Setup       Uninstall All
Test Teardown    Run Keywords
...              EDR And Base Teardown  AND
...              Uninstall EDR

Library    ../Libs/InstallerUtils.py

Resource    EDRResources.robot

Force Tags  TAP_PARALLEL2

*** Test Cases ***
EDR Installer Directories And Files
## -------------------------------------READ----ME------------------------------------------------------
## Please note that these tests rely on the files in InstallSet being upto date. To regenerate these run
## an install manually and run the generateFromInstallDir.sh from InstallSet directory.
## WARNING
## If you generate this from a local build please make sure that you have blatted the distribution
## folder before remaking it. Otherwise old content can slip through to new builds and corrupt the
## fileset.
## ENSURE THAT THE CHANGES YOU SEE IN THE COMMIT DIFF ARE WHAT YOU WANT
## -----------------------------------------------------------------------------------------------------
    [Teardown]    EDR Tests Teardown With Installed File Replacement
    Install With Base SDDS
    Check EDR Plugin Installed With Base
    Wait Until Keyword Succeeds
        ...  30 secs
        ...  1 secs
        ...  EDR Plugin Log Contains      Creating osquery options config file

    ${DirectoryInfo}  ${FileInfo}  ${SymbolicLinkInfo} =  Get File Info For Installation
    Set Test Variable  ${FileInfo}
    Set Test Variable  ${DirectoryInfo}
    Set Test Variable  ${SymbolicLinkInfo}

    # Check Directory Structure
    Log  ${DirectoryInfo}
    ${ExpectedDirectoryInfo}=  Get File  ${INSTALL_SET_PATH}/DirectoryInfo
    Should Be Equal As Strings  ${ExpectedDirectoryInfo}  ${DirectoryInfo}

    # Check File Info
    ${ExpectedFileInfo}=  Get File  ${INSTALL_SET_PATH}/FileInfo
    Should Be Equal As Strings  ${ExpectedFileInfo}  ${FileInfo}

    # Check Symbolic Links
    ${ExpectedSymbolicLinkInfo} =  Get File  ${INSTALL_SET_PATH}/SymbolicLinkInfo
    Should Be Equal As Strings  ${ExpectedSymbolicLinkInfo}  ${SymbolicLinkInfo}

*** Keywords ***
EDR Tests Teardown With Installed File Replacement
    Run Keyword If Test Failed  Save Current EDR InstalledFiles To Local Path
    Check For Osquery Errors Or Warnings And Log File Contents
    EDR And Base Teardown
    Uninstall All
    Unmount All Comms Component Folders

Save Current EDR InstalledFiles To Local Path
    Create File  ${INSTALL_SET_PATH}/FileInfo  ${FileInfo}
    Create File  ${INSTALL_SET_PATH}/DirectoryInfo  ${DirectoryInfo}
    Create File  ${INSTALL_SET_PATH}/SymbolicLinkInfo  ${SymbolicLinkInfo}

Check For Osquery Errors Or Warnings And Log File Contents
    ${result} =   Run Process  find ${COMPONENT_ROOT_PATH}/log/osqueryd.*   shell=True
    Log  ${result.stdout}
    Log  ${result.stderr}
    ${result} =   Run Process  cat ${COMPONENT_ROOT_PATH}/log/osqueryd.*   shell=True
    Log  ${result.stdout}
    Log  ${result.stderr}
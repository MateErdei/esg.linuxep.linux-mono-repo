*** Settings ***
Resource    DeviceIsolationResources.robot
Library     ${COMMON_TEST_LIBS}/FullInstallerUtils.py
Library     ${COMMON_TEST_LIBS}/LogUtils.py
Library     Collections

Suite Setup     Device Isolation Suite Setup
Suite Teardown  Device Isolation Suite Teardown

Test Setup     Device Isolation Test Setup
Test Teardown  Install Device Isolation Test Teardown

*** Test Cases ***
Verify that the device isolation installer works correctly
    [Tags]    EXCLUDE_ON_COVERAGE
## -------------------------------------READ----ME------------------------------------------------------
## Please note that these tests rely on the files in InstallSet being upto date. To regenerate these run
## an install manually and run the generateFromInstallDir.sh from InstallSet directory.
## WARNING
## If you generate this from a local build please make sure that you have blatted the distribution
## folder before remaking it. Otherwise old content can slip through to new builds and corrupt the
## fileset.
## ENSURE THAT THE CHANGES YOU SEE IN THE COMMIT DIFF ARE WHAT YOU WANT
## -----------------------------------------------------------------------------------------------------
    Register Cleanup  Save Current Device Isolation InstalledFiles To Local Path
    Install Device Isolation Directly from SDDS
    Wait Until Created    ${DEVICE_ISOLATION_LOG_PATH}

    ${DirectoryInfo}  ${FileInfo}  ${SymbolicLinkInfo} =   Get File Info For Installation  ${COMPONENT_NAME}
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
    Install Device Isolation Directly from SDDS
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

Verify That Device Isolation Logging Can Be Set Individually
    Create File         ${SOPHOS_INSTALL}/base/etc/logger.conf.local   [${COMPONENT_NAME}]\nVERBOSITY=DEBUG\n
    ${ej_mark} =    Get Device Isolation Log Mark
    Install Device Isolation Directly from SDDS
    Wait For Log Contains From Mark    ${ej_mark}    Logger ${COMPONENT_NAME} configured for level: DEBUG

*** Keywords ***
Install Device Isolation Test Teardown
    Device Isolation Test Teardown
    Uninstall Device Isolation

Save Current Device Isolation InstalledFiles To Local Path
    Create File  ${ROBOT_SCRIPTS_PATH}/InstallSet/FileInfo  ${FileInfo}
    Create File  ${ROBOT_SCRIPTS_PATH}/InstallSet/DirectoryInfo  ${DirectoryInfo}
    Create File  ${ROBOT_SCRIPTS_PATH}/InstallSet/SymbolicLinkInfo  ${SymbolicLinkInfo}

*** Settings ***
Documentation    Suite description

Resource  ${COMMON_TEST_ROBOT}/InstallerResources.robot
Resource  ${COMMON_TEST_ROBOT}/LiveResponseResources.robot
Resource  ${COMMON_TEST_ROBOT}/WatchdogResources.robot

Suite Setup     Require Fresh Install
Suite Teardown  Require Uninstalled

Test Teardown  General Test Teardown

Default Tags   LIVERESPONSE_PLUGIN


*** Test Cases ***
Verify that the live response installer works correctly
## -------------------------------------READ----ME------------------------------------------------------
## Please note that these tests rely on the files in InstallSet being upto date. To regenerate these run
## an install manually and run the generateFromInstallDir.sh from InstallSet directory.
## WARNING
## If you generate this from a local build please make sure that you have blatted the distribution
## folder before remaking it. Otherwise old content can slip through to new builds and corrupt the
## fileset.
## ENSURE THAT THE CHANGES YOU SEE IN THE COMMIT DIFF ARE WHAT YOU WANT
## -----------------------------------------------------------------------------------------------------
    [Teardown]  Live Response Tests Teardown With Installed File Replacement
    Install Live Response Directly

    ${DirectoryInfo}  ${FileInfo}  ${SymbolicLinkInfo} =   get file info for installation  liveresponse
    Set Test Variable  ${FileInfo}
    Set Test Variable  ${DirectoryInfo}
    Set Test Variable  ${SymbolicLinkInfo}
    ## Check Directory Structure
    Log  ${DirectoryInfo}
    ${ExpectedDirectoryInfo}=  Get File  ${ROBOT_TESTS_DIR}/liveresponse_plugin/InstallSet/DirectoryInfo
    Should Be Equal As Strings  ${ExpectedDirectoryInfo}  ${DirectoryInfo}

    ## Check File Info
    # wait for /opt/sophos-spl/base/mcs/status/cache/ALC.xml to exist
    ${ExpectedFileInfo}=  Get File  ${ROBOT_TESTS_DIR}/liveresponse_plugin/InstallSet/FileInfo
    Should Be Equal As Strings  ${ExpectedFileInfo}  ${FileInfo}

    ## Check Symbolic Links
    ${ExpectedSymbolicLinkInfo} =  Get File  ${ROBOT_TESTS_DIR}/liveresponse_plugin/InstallSet/SymbolicLinkInfo
    Should Be Equal As Strings  ${ExpectedSymbolicLinkInfo}  ${SymbolicLinkInfo}

Verify That Live Response Logging Can Be Set Individually
    [Teardown]  Live Response Tests Teardown With Installed File Replacement
    Install Live Response Directly
    Create File         ${SOPHOS_INSTALL}/base/etc/logger.conf.local   [liveresponse]\nVERBOSITY=DEBUG\n
    Remove File         ${SOPHOS_INSTALL}/plugins/liveresponse/log/liveresponse.log
    Restart Liveresponse Plugin
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Live Response Plugin Log Contains  Logger liveresponse configured for level: DEBUG

Live Response Plugin Installs With Version Ini File
    Run Full Installer
    Install Live Response Directly
    File Should Exist   ${SOPHOS_INSTALL}/plugins/liveresponse/VERSION.ini
    VERSION Ini File Contains Proper Format For Product Name   ${SOPHOS_INSTALL}/plugins/liveresponse/VERSION.ini   SPL-Live-Response-Plugin


*** Keywords ***
Live Response Tests Teardown With Installed File Replacement
    Run Keyword If Test Failed  Save Current Live Response InstalledFiles To Local Path

Save Current Live Response InstalledFiles To Local Path
    Create File  ${ROBOT_TESTS_DIR}/liveresponse_plugin/InstallSet/FileInfo  ${FileInfo}
    Create File  ${ROBOT_TESTS_DIR}/liveresponse_plugin/InstallSet/DirectoryInfo  ${DirectoryInfo}
    Create File  ${ROBOT_TESTS_DIR}/liveresponse_plugin/InstallSet/SymbolicLinkInfo  ${SymbolicLinkInfo}
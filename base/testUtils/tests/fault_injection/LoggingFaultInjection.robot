*** Settings ***
Documentation    Suite description


Library     ${LIBS_DIRECTORY}/TemporaryDirectoryManager.py

Resource    ../installer/InstallerResources.robot

Test Setup  Local Test Setup
Test Teardown  Local Test Teardown

Suite Setup   Local Suite Setup
Suite Teardown   Local Suite Teardown

Default Tags   FAULTINJECTION

*** Variables ***
${MDR_PLUGIN_PATH}              ${SOPHOS_INSTALL}/plugins/mtr/
${sulConfigPath}                ${SOPHOS_INSTALL}/base/update/var/update_config.json
${MDR_VUT_POLICY}               ${SUPPORT_FILES}/CentralXml/RealWarehousePolicies/GeneratedAlcPolicies/base_and_mtr_VUT.xml
${FakeLogger}                   SystemProductTestOutput/FaultInjectionLogger
${FakeLogger}                   SystemProductTestOutput/FaultInjectionLogger
${StockLoggerConfLocation}      /opt/sophos-spl/base/etc/logger.conf.0
${LowPrivilegeLogLocation}      /tmp/test.log
${HighPrivilegeDirectory}       /tmp/HighPrivilegeDirectory
${HighPrivilegeLogLocation}     ${HighPrivilegeDirectory}/test.log

*** Test Cases ***
Logger With Removed Logger Conf Uses Default Configuration
    Remove File  ${StockLoggerConfLocation}
    ${fakeLoggerOutput} =  Run Fake Logger  10  ${LowPrivilegeLogLocation}
    Should Contain  ${fakeLoggerOutput}  Failed to read the config file /opt/sophos-spl/base/etc/logger.conf. All settings will be set to their default value
    Validate Fake Log  ${LowPrivilegeLogLocation}  10  INFO

Logger With No Access To Logger Conf Uses Default Configuration
    Remove File  ${StockLoggerConfLocation}
    Create Directory  ${StockLoggerConfLocation}
    ${fakeLoggerOutput} =  Run Fake Logger As Low Priveliged User  10  ${LowPrivilegeLogLocation}
    Should Contain  ${fakeLoggerOutput}  Failed to read the config file /opt/sophos-spl/base/etc/logger.conf. All settings will be set to their default value
    Validate Fake Log  ${LowPrivilegeLogLocation}  10  INFO

Logger Does Not Have Permission To Write To Target Log Path
    Create High Privilege Directory
    ${fakeLoggerOutput} =  Run Fake Logger As Low Priveliged User  10  ${HighPrivilegeLogLocation}
    Should Contain  ${fakeLoggerOutput}  log4cplus:ERROR Unable to open file: ${HighPrivilegeLogLocation}
    Should Not Exist  ${HighPrivilegeLogLocation}

Logger Cannot Write To Target Log Path Because It Is A Directory
    Create High Privilege Directory
    ${fakeLoggerOutput} =  Run Fake Logger  10  ${HighPrivilegeDirectory}
    Should Not Exist  ${HighPrivilegeLogLocation}
    Should Contain  ${fakeLoggerOutput}  log4cplus:ERROR Unable to open file: ${HighPrivilegeDirectory}

Logger Does Not Hang When It Runs Out Of Disk Space To Write To
    [Teardown]  Local Test Teardown With Test File System Cleanup
    ${FileSystemRoot} =  Make Test Filesystem  1000K
    ${SmallFileSystemLogFile} =  Set Variable  ${FileSystemRoot}/test.log
    Create File Of Size  700000  ${FileSystemRoot}/bigFile
    ${fakeLoggerOutput} =  Run Fake Logger  10000  ${SmallFileSystemLogFile}
    ${LogContents} =  Get File  ${SmallFileSystemLogFile}
    Should Exist  ${SmallFileSystemLogFile}
    Should Not Contain  ${LogContents}  testlogger <> Write this boring line10000
    File System Should Be Full  ${FileSystemRoot}

*** Keywords ***

Local Test Setup
    Require Installed

Local Test Teardown
    General Test Teardown
    Run Keyword And Ignore Error  Remove High Privilege Directory
    Run Keyword And Ignore Error  Remove File  ${LowPrivilegeLogLocation}
    Run Keyword If Test Failed    Require Fresh Install

Local Test Teardown With Test File System Cleanup
    Local Test Teardown
    Cleanup Test Filesystem

Local Suite Setup
    Require Fresh Install
    Chmod  a+x  ${FakeLogger}

Local Suite Teardown
    Ensure Uninstalled

Run Fake Logger
    [Arguments]  ${numLines}  ${outputPath}
    ${result} =  Run Process  ${FakeLogger}  ${numLines}  ${outputPath}  env:LD_LIBRARY_PATH=/opt/sophos-spl/base/lib64/
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Strings  ${result.rc}  0
    [Return]  ${result.stdout}\n${result.stderr}

Run Fake Logger As Low Priveliged User
    [Arguments]  ${numLines}  ${outputPath}
    ${result} =  Run Process  sudo -u sophos-spl-user LD_LIBRARY_PATH\=/opt/sophos-spl/base/lib64/ ${FakeLogger} ${numLines} ${outputPath}  shell=True
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Strings  ${result.rc}  0
    Should Contain  ${result.stdout}  Finished Execution
    [Return]  ${result.stdout}\n${result.stderr}

Create High Privilege Directory
    Create Directory  ${HighPrivilegeDirectory}
    Chmod  700  ${HighPrivilegeDirectory}
    Should Exist  ${HighPrivilegeDirectory}

Remove High Privilege Directory
    remove directory  ${HighPrivilegeDirectory}  recursive=True

Chmod
    [Arguments]  ${permissions}  ${Path}
    Should Exist  ${Path}
    ${result} =  Run Process  chmod  ${permissions}  ${Path}
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Strings  ${result.rc}  0

Validate Fake Log
    [Arguments]  ${LogPath}  ${NumLines}  ${LogLevel}=INFO
    file Should Exist  ${LogPath}
    ${LogContents} =  Get File  ${LogPath}
    Should Contain  ${LogContents}  testlogger <> Write this boring line${NumLines}
    Should Contain  ${LogContents}  root <> Logger testlogger configured for level: ${LogLevel}

Make Test Filesystem
    [Arguments]  ${Size}
    ${workspace} =  Add Temporary Directory  FileSystemDirectory
    ${FileSystemFile} =  Set Variable  ${workspace}/FileSystem
    Set Test Variable  ${MountPoint}  ${workspace}/MountPoint
    Create Directory  ${MountPoint}
    Create File     ${FileSystemFile}
    Set File Size   ${Size}  ${FileSystemFile}
    Turn File Into File System  ${FileSystemFile}
    Mount File As Filesystem  ${FileSystemFile}  ${MountPoint}
    # return the root of the new mounted filesystem
    [Return]  ${MountPoint}

Set File Size
    [Arguments]  ${Size}  ${FilePath}
    File Should Exist  ${FilePath}
    ${result} =  Run Process   truncate   -s   ${Size}   ${FilePath}
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Strings  ${result.rc}  0

Turn File Into File System
    [Arguments]  ${FilePath}
    ${result} =  Run Process   mke2fs   -t   ext4   -F   ${FilePath}
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Strings  ${result.rc}  0

Mount File As Filesystem
    [Arguments]  ${FilesystemFile}  ${DirectoryToMountFileSystemOn}
    ${result} =  Run Process   mount   ${FilesystemFile}  ${DirectoryToMountFileSystemOn}
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Strings  ${result.rc}  0

Unmount Filesystem From Directory
    [Arguments]  ${Directory}
    Directory Should Exist  ${Directory}
    ${result} =  Run Process  umount  ${Directory}
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Strings  ${result.rc}  0

Cleanup Test Filesystem
    Unmount Filesystem From Directory  ${MountPoint}
    Cleanup Temporary Folders

Create File Of Size
    [Arguments]  ${size}  ${location}
    ${result} =  Run Process  dd if\=/dev/urandom of\=${location} bs\=${size} count\=1    shell=True
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Strings  ${result.rc}  0

File System Should Be Full
    [Arguments]  ${MountDirectory}
    ${result} =  Run Process  df  -h
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Strings  ${result.rc}  0
    Should Contain  ${result.stdout}  0 100% ${MountDirectory}
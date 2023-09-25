*** Settings ***
Library     Process

Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library    ${COMMON_TEST_LIBS}/LogUtils.py
Library    ${LIBS_DIRECTORY}/OSUtils.py

Resource  DiagnoseResources.robot
Resource  ../GeneralTeardownResource.robot
Resource  ../mcs_router/McsRouterResources.robot

Suite Setup  Require Fresh Install
Suite Teardown  Run Keywords
...             Ensure Uninstalled  AND
...             Cleanup Certificates

*** Variables ***
${RESPONSE_ACTIONS_LOG_PATH}   ${SOPHOS_INSTALL}/plugins/responseactions/log/responseactions.log


*** Test Cases ***
Test Zip And Unzip Directory
    Create Directory   ${TAR_FILE_DIRECTORY}
    Create File   ${TAR_FILE_DIRECTORY}/test.txt   this is a test file
    ${preZipMode} =    Set Variable    764
    Run Process  chmod  ${preZipMode}  ${TAR_FILE_DIRECTORY}/test.txt
    ${result}=    Run Process  ls -l ${TAR_FILE_DIRECTORY}/test.txt  shell=True
    Log  ${result.stdout}

    ${zipTool} =  Set Variable  SystemProductTestOutput/zipTool
    ${unzipTool} =  Set Variable  SystemProductTestOutput/unzipTool
    ${zipFile} =  Set Variable  /tmp/test.zip

    Remove File  ${zipFile}
    ${result} =    Run Process  LD_LIBRARY_PATH\=/opt/sophos-spl/base/lib64/ ${zipTool} ${TAR_FILE_DIRECTORY} ${zipFile}   shell=True
    Log  ${result.stderr}
    Log  ${result.stdout}
    Should Be Equal As Integers    ${result.rc}    0   "zip utility failed: Reason ${result.stderr}"
    Should Exist  ${zipFile}

    Create Directory  ${UNPACK_DIRECTORY}
    Empty Directory  ${UNPACK_DIRECTORY}
    ${result} =    Run Process  LD_LIBRARY_PATH\=/opt/sophos-spl/base/lib64/ ${unzipTool} ${zipFile} ${UNPACK_DIRECTORY}   shell=True
    Log  ${result.stderr}
    Log  ${result.stdout}
    Should Be Equal As Integers    ${result.rc}    0   "zip utility failed: Reason ${result.stderr}"
    Directory Should Not Be Empty  ${UNPACK_DIRECTORY}
    ${extractedModTime} =    Get Last Modified Time    ${UNPACK_DIRECTORY}/TestOutputDirectory/test.txt
    ${result}=    Run Process  ls -l ${UNPACK_DIRECTORY}/TestOutputDirectory/test.txt  shell=True
    Log  ${result.stdout}
    Should Not Be Equal As Integers    ${extractedModTime}    ${0}    "Timestamp or extracted file not restored"
    Ensure Chmod File Matches    ${UNPACK_DIRECTORY}/TestOutputDirectory/test.txt    ${preZipMode}

Test Zip And Unzip Directory with trailing slash
    Create Directory   ${TAR_FILE_DIRECTORY}
    Create File   ${TAR_FILE_DIRECTORY}/test.txt   this is a test file
    ${preZipMode} =    Set Variable    764
    Run Process  chmod  ${preZipMode}  ${TAR_FILE_DIRECTORY}/test.txt
    ${result}=    Run Process  ls -l ${TAR_FILE_DIRECTORY}/test.txt  shell=True
    Log  ${result.stdout}

    ${zipTool} =  Set Variable  SystemProductTestOutput/zipTool
    ${unzipTool} =  Set Variable  SystemProductTestOutput/unzipTool
    ${zipFile} =  Set Variable  /tmp/test.zip

    Remove File  ${zipFile}
    ${result} =    Run Process  LD_LIBRARY_PATH\=/opt/sophos-spl/base/lib64/ ${zipTool} ${TAR_FILE_DIRECTORY}/ ${zipFile}   shell=True
    Log  ${result.stderr}
    Log  ${result.stdout}
    Should Be Equal As Integers    ${result.rc}    0   "zip utility failed: Reason ${result.stderr}"
    Should Exist  ${zipFile}

    Create Directory  ${UNPACK_DIRECTORY}
    Empty Directory  ${UNPACK_DIRECTORY}
    ${result} =    Run Process  LD_LIBRARY_PATH\=/opt/sophos-spl/base/lib64/ ${unzipTool} ${zipFile} ${UNPACK_DIRECTORY}   shell=True
    Log  ${result.stderr}
    Log  ${result.stdout}
    Should Be Equal As Integers    ${result.rc}    0   "zip utility failed: Reason ${result.stderr}"
    Directory Should Not Be Empty  ${UNPACK_DIRECTORY}
    ${extractedModTime} =    Get Last Modified Time    ${UNPACK_DIRECTORY}/TestOutputDirectory/test.txt
    ${result}=    Run Process  ls -l ${UNPACK_DIRECTORY}/TestOutputDirectory/test.txt  shell=True
    Log  ${result.stdout}
    Should Not Be Equal As Integers    ${extractedModTime}    ${0}    "Timestamp or extracted file not restored"
    Ensure Chmod File Matches    ${UNPACK_DIRECTORY}/TestOutputDirectory/test.txt    ${preZipMode}
Test Zip And Unzip Directory With Password
    Create Directory   ${TAR_FILE_DIRECTORY}
    Create File   ${TAR_FILE_DIRECTORY}/test.txt   this is a test file

    ${zipTool} =  Set Variable  SystemProductTestOutput/zipTool
    ${unzipTool} =  Set Variable  SystemProductTestOutput/unzipTool
    ${zipFile} =  Set Variable  /tmp/test.zip
    ${password} =  Set Variable  password

    Remove File  ${zipFile}
    ${result} =    Run Process  LD_LIBRARY_PATH\=/opt/sophos-spl/base/lib64/ ${zipTool} ${TAR_FILE_DIRECTORY} ${zipFile} ${password}   shell=True
    Log  ${result.stderr}
    Log  ${result.stdout}
    Should Be Equal As Integers    ${result.rc}    0   "zip failed: Reason ${result.stderr}"
    Should Exist  ${zipFile}

    Create Directory  ${UNPACK_DIRECTORY}
    Empty Directory  ${UNPACK_DIRECTORY}
    ${result} =    Run Process  LD_LIBRARY_PATH\=/opt/sophos-spl/base/lib64/ ${unzipTool} ${zipFile} ${UNPACK_DIRECTORY}   shell=True
    Log  ${result.stderr}
    Log  ${result.stdout}
    ${Files} =  List Files In Directory  ${UNPACK_DIRECTORY}
    Log   ${Files}
    Should Not Be Equal As Integers    ${result.rc}    0   "unzip utility unexpectedly succeeded without password"

    ${result} =    Run Process  LD_LIBRARY_PATH\=/opt/sophos-spl/base/lib64/ ${unzipTool} ${zipFile} ${UNPACK_DIRECTORY} ${password}   shell=True
    Should Be Equal As Integers    ${result.rc}    0   "unzip failed: Reason ${result.stderr}"
    Log  ${result.stderr}
    Log  ${result.stdout}
    Directory Should Not Be Empty  ${UNPACK_DIRECTORY}

*** Settings ***
Library     OperatingSystem
Library     Process

Library    ${COMMON_TEST_LIBS}/FullInstallerUtils.py
Library    ${COMMON_TEST_LIBS}/FileUtils.py
Library    ${COMMON_TEST_LIBS}/LogUtils.py
Library    ${COMMON_TEST_LIBS}/OnFail.py
Library    ${COMMON_TEST_LIBS}/OSUtils.py

Resource   ${COMMON_TEST_ROBOT}/DiagnoseResources.robot
Resource   ${COMMON_TEST_ROBOT}/GeneralTeardownResource.robot
Resource   ${COMMON_TEST_ROBOT}/McsRouterResources.robot

Suite Setup  Require Fresh Install
Suite Teardown  Ensure Uninstalled

Test Setup    TestZipUtils Setup
Test Teardown    TestZipUtils Teardown

Force Tags    TAP_PARALLEL4

*** Variables ***
${RESPONSE_ACTIONS_LOG_PATH}   ${SOPHOS_INSTALL}/plugins/responseactions/log/responseactions.log
${ZIP_TOOL}      ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}/zipTool
${UNZIP_TOOL}    ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}/unzipTool

*** Test Cases ***
Test Zip And Unzip Directory
    ${preZipMode} =    Set Variable    764
    Run Process  chmod  ${preZipMode}  ${TAR_FILE_DIRECTORY}/test.txt
    ${result}=    Run Process  ls -l ${TAR_FILE_DIRECTORY}/test.txt  shell=True
    Log  ${result.stdout}

    ${zipFile} =  Set Variable  /tmp/test1.zip

    ${result} =    Run Process  LD_LIBRARY_PATH\=/opt/sophos-spl/base/lib64/ ${ZIP_TOOL} ${TAR_FILE_DIRECTORY} ${zipFile}   shell=True
    Log  ${result.stderr}
    Log  ${result.stdout}
    Should Be Equal As Integers    ${result.rc}    0   "zip utility failed: Reason ${result.stderr}"
    Should Exist  ${zipFile}

    ${result} =    Run Process  LD_LIBRARY_PATH\=/opt/sophos-spl/base/lib64/ ${UNZIP_TOOL} ${zipFile} ${UNPACK_DIRECTORY}   shell=True
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
    ${preZipMode} =    Set Variable    764
    Run Process  chmod  ${preZipMode}  ${TAR_FILE_DIRECTORY}/test.txt
    ${result}=    Run Process  ls -l ${TAR_FILE_DIRECTORY}/test.txt  shell=True
    Log  ${result.stdout}

    ${zipFile} =  Set Variable  /tmp/test2.zip

    ${result} =    Run Process  LD_LIBRARY_PATH\=/opt/sophos-spl/base/lib64/ ${ZIP_TOOL} ${TAR_FILE_DIRECTORY}/ ${zipFile}   shell=True
    Log  ${result.stderr}
    Log  ${result.stdout}
    Should Be Equal As Integers    ${result.rc}    0   "zip utility failed: Reason ${result.stderr}"
    Should Exist  ${zipFile}

    ${result} =    Run Process  LD_LIBRARY_PATH\=/opt/sophos-spl/base/lib64/ ${UNZIP_TOOL} ${zipFile} ${UNPACK_DIRECTORY}   shell=True
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
    ${zipFile} =  Set Variable  /tmp/test3.zip
    ${password} =  Set Variable  password

    ${result} =    Run Process  ${ZIP_TOOL}  ${TAR_FILE_DIRECTORY}  ${zipFile}  ${password}   env:LD_LIBRARY_PATH=/opt/sophos-spl/base/lib64/  stderr=STDOUT
    Log  ${result.stdout}
    Should Be Equal As Integers    ${result.rc}    ${0}   "zip failed: Reason ${result.stderr}"
    Should Exist  ${zipFile}
    Assert zip file is password protected  ${zipFile}

    ${result} =    Run Process  ${UNZIP_TOOL}  ${zipFile}  ${UNPACK_DIRECTORY}   env:LD_LIBRARY_PATH=/opt/sophos-spl/base/lib64/  stderr=STDOUT
    Log   unziptool output: ${result.stdout}
    ${Files} =  List Directory  ${UNPACK_DIRECTORY}
    Log   List Directory ${Files}
    ${lsOutput} =  Run Process  ls  -lR   ${UNPACK_DIRECTORY}   stderr=STDOUT
    Log   ls -lR ${lsOutput.stdout}
    Should Not Be Equal As Integers    ${result.rc}    ${0}   "unzip utility unexpectedly succeeded without password"

    ${result} =    Run Process  ${UNZIP_TOOL}  ${zipFile}  ${UNPACK_DIRECTORY}  ${password}   env:LD_LIBRARY_PATH=/opt/sophos-spl/base/lib64/  stderr=STDOUT
    Should Be Equal As Integers    ${result.rc}    ${0}   "unzip failed: Reason ${result.stderr}"
    Log  ${result.stdout}
    Directory Should Not Be Empty  ${UNPACK_DIRECTORY}

*** Keywords ***
TestZipUtils Setup
    Create Directory   ${TAR_FILE_DIRECTORY}
    Create Directory   ${UNPACK_DIRECTORY}
    Register Cleanup    Remove Directory    ${TAR_FILE_DIRECTORY}  recursive=True
    Register Cleanup    Remove Directory    ${UNPACK_DIRECTORY}  recursive=True
    Create File   ${TAR_FILE_DIRECTORY}/test.txt   this is a test file

TestZipUtils Teardown
    Run Teardown Functions
    General Test Teardown

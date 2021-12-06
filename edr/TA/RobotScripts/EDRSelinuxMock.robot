*** Settings ***
Library         Process
Library         OperatingSystem
Library         ../Libs/FakeManagement.py
Library         ../Libs/UserUtils.py
Library         ../Libs/FileSystemLibs.py

Resource    ComponentSetup.robot
Resource    EDRResources.robot

Test Setup     Install Base For Component Tests
Test Teardown  Test Teardown

Suite Teardown  Uninstall All

*** Variables ***
${BACKUP_SUFFIX}  .back

*** Test Cases ***

EDR Installer Calls Semanage on Shared Log When Selinux And Semanage Are Installed
    Create Fake System Executable  getenforce
    Create Fake System Executable  semanage
    Create Fake System Executable  restorecon

    ${installer_output} =  Install EDR Directly from SDDS
    ${logFile} =  Get File  /tmp/mockedExecutable

    Should Contain  ${logFile}  semanage fcontext -a -t var_log_t ${SOPHOS_INSTALL}/shared/syslog_pipe
    Should Contain  ${logFile}  restorecon -Fv ${SOPHOS_INSTALL}/shared/syslog_pipe

EDR Does Not Set Selinux Context When Selinux Is Not Detected
    Obscure System Executable  getenforce

    ${installer_output} =  Install EDR Directly from SDDS
    File Should Not Exist  /tmp/mockedExecutable
    Should Not Contain  ${installer_output}   semanage

EDR Installer Logs Warning When Semanage Is Missing
    Create Fake System Executable  getenforce
    Create Fake System Executable  restorecon
    Obscure System Executable  semanage

    ${installer_output} =  Install EDR Directly from SDDS

    Should Contain  ${installer_output}  WARNING: Detected selinux is present on system, but could not find semanage to setup syslog pipe, osquery will not be able to receive syslog events

EDR Installer Logs Warning When Semanage Fails
    Create Fake System Executable  getenforce
    Create Fake System Executable  restorecon
    Create Fake System Executable  semanage  mock_file=${EXAMPLE_DATA_PATH}/FailingMockedExecutable.sh

    ${installer_output} =  Install EDR Directly from SDDS
    ${logFile} =  Get File  /tmp/mockedExecutable

    Should Contain  ${installer_output}  WARNING: Failed to setup syslog pipe, osquery will not able to receive syslog events
    Should Not Contain  ${installer_output}  semanage fcontext -a -t var_log_t /opt/sophos-spl/shared/syslog_pipe

No Stdout Or Stderr Comes From Which When Called
    [Teardown]  Fix Mocked Which Teardown
    Create Fake System Executable  which  mock_file=${EXAMPLE_DATA_PATH}/MockedExecutableStdoutAndStderr.sh  which_basename=which${BACKUP_SUFFIX}
    ${installer_output} =  Install EDR Directly from SDDS
    Should Not Contain  ${installer_output}  this is mocked stdout from which
    Should Not Contain  ${installer_output}  this is mocked stderr from which

    ${uninstaller_output} =  Uninstall EDR
    Should Not Contain  ${uninstaller_output}  this is mocked stdout from which
    Should Not Contain  ${uninstaller_output}  this is mocked stderr from which

*** Keywords ***
Fix Mocked Which Teardown
    ${result} =  Run Process  which.back  which.back
    Move File  ${result.stdout}  ${result.stdout[:-5]}
    Test Teardown

Test Teardown
    Common Teardown
    Uninstall All
    Restore System Executable  getenforce
    Restore System Executable  semanage
    Restore System Executable  restorecon
    Remove File  /tmp/mockedExecutable


Backup System Executable
    [Arguments]  ${path}
    Move File  ${path}  ${path}${BACKUP_SUFFIX}

Restore System Executable
    [Arguments]  ${executable_name}
    ${executable_path} =  Get Path To Executable  ${executable_name}
    ${fileExists} =  Does File Exist  ${executable_path}${BACKUP_SUFFIX}
    Run Keyword If    ${fileExists} is True
    ...  Move File  ${executable_path}${BACKUP_SUFFIX}  ${executable_path}
    ...  ELSE IF  "${executable_path}" != ""
    ...  Remove File  ${executable_path}

Get Path To Executable
    [Arguments]  ${executable_name}
    ${result} =  Run Process  which  ${executable_name}
    [Return]  ${result.stdout}

Obscure System Executable
    [Arguments]  ${executable_name}
    ${executable_path} =  Get Path To Executable  ${executable_name}
    Run Keyword If   "${executable_path}" != ""
    ...  Backup System Executable  ${executable_path}

Create Fake System Executable
    [Arguments]  ${executable_name}  ${mock_file}=${EXAMPLE_DATA_PATH}/MockedExecutable.sh  ${which_basename}=which
    ${executable_path} =  Get Path To Executable  ${executable_name}
    Run Keyword If   "${executable_path}" != ""
    ...  Backup System Executable  ${executable_path}
    ${executable_path} =  Set Variable If  "${executable_path}" == ""
    ...  /usr/bin/${executable_name}
    ...  ${executable_path}
    log to console  copying ${mock_file} to ${executable_path}
    Copy File  ${mock_file}  ${executable_path}
    ${x} =  Run Process  chmod  +x  ${executable_path}
    Should Be Equal As Integers  ${x.rc}  0
    # Check which finds new executable
    ${x} =  Run Process  ${which_basename}  ${executable_name}
    Should Be Equal As Integers  ${x.rc}  0
    Should Be Equal As Strings  ${x.stdout}  ${executable_path}

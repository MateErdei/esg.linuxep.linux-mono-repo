*** Settings ***
Library         Process
Library         OperatingSystem
Library         ../Libs/FakeManagement.py
Library         ../Libs/UserUtils.py
Library         ../Libs/FileSystemLibs.py
Library         ../Libs/fixtures/EDRPlugin.py

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

    ${installer_stdout} =  Install EDR Directly from SDDS
    Log To Console  ${installer_stdout}

    Should Contain  ${installer_stdout}  semanage fcontext -a -t var_log_t /opt/sophos-spl/shared/syslog_pipe
    Should Contain  ${installer_stdout}  restorecon -Fv /opt/sophos-spl/shared/syslog_pipe

EDR Does Not Set Selinux Context When Selinux Is Not Detected
    Obscure System Executable  getenforce

    ${installer_stdout} =  Install EDR Directly from SDDS
    Log To Console  ${installer_stdout}

    Should Not Contain  ${installer_stdout}  semanage
    Should Not Contain  ${installer_stdout}  restorecon

EDR Installer Logs Warning When Semanage Is Missing
    Create Fake System Executable  getenforce
    Create Fake System Executable  restorecon
    Obscure System Executable  semanage

    ${installer_stdout} =  Install EDR Directly from SDDS
    Log To Console  ${installer_stdout}

    Should Contain  ${installer_stdout}  WARNING: Detected selinux is present on system, but could not find semanage executable to add var_log_t context to /opt/sophos-spl/shared/syslog_pipe

EDR Installer Logs Warning When Semanage Fails
    Create Fake System Executable  getenforce
    Create Fake System Executable  restorecon
    Create Fake System Executable  semanage  mock_file=${EXAMPLE_DATA_PATH}/FailingMockedExecutable.sh

    ${installer_stdout} =  Install EDR Directly from SDDS
    Log To Console  ${installer_stdout}

    Should Contain  ${installer_stdout}  WARNING: Failed to add var_log_t context to /opt/sophos-spl/shared/syslog_pipe

*** Keywords ***
Test Teardown
    Uninstall All
    Restore System Executable  getenforce
    Restore System Executable  semanage
    Restore System Executable  restorecon


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
    [Arguments]  ${executable_name}  ${mock_file}=${EXAMPLE_DATA_PATH}/MockedExecutable.sh
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
    ${x} =  Run Process  which  ${executable_name}
    Should Be Equal As Integers  ${x.rc}  0
    Should Be Equal As Strings  ${x.stdout}  ${executable_path}


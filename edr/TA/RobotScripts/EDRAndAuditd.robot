*** Settings ***
Documentation    Tests relating to EDR and audit subsystem.

Library         Process
Library         OperatingSystem
Library         String
Library         ../Libs/OSLibs.py

Resource        EDRResources.robot

Suite Setup      Suite Setup For EDR And Auditd
Suite Teardown   Suite Teardown For EDR and Auditd

Test Setup      No Operation
Test Teardown   Test Teardown For EDR And Auditd

Default Tags    TAP_PARALLEL1

*** Test Cases ***
EDR By Default Will Configure Audit Option
    ${is_suse} =  Does File Contain Word  /etc/os-release  SUSE Linux Enterprise Server 12
    Pass Execution If  ${is_suse}  LINUXDAR-7106 - test broken on SLES12 - wont be fixed
    Ensure AuditD Running
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check AuditD Executable Running

    Install Base For Component Tests
    Install EDR Directly from SDDS
    Check EDR Plugin Installed With Base
    Wait Until Keyword Succeeds
    ...   20 secs
    ...   2 secs
    ...   Check Osquery Running
    Check EDR Plugin Config Contains  disable_auditd=1
    # Check osquery flags file has been set correctly
    ${contents}=  Get File  ${EDR_PLUGIN_PATH}/etc/osquery.flags
    Should contain  ${contents}  --disable_audit=false
    Should contain  ${contents}  --audit_allow_process_events=true
    Should contain  ${contents}  --audit_allow_sockets=false
    Should contain  ${contents}  --audit_allow_user_events=true

    # Default behaviour is that Auditd will be stopped and disabled.
    Check AuditD Executable Not Running
    Check AuditD Service Disabled
    Check EDR Log Shows AuditD Has Been Disabled
    Wait Until Keyword Succeeds
    ...   20 secs
    ...   2 secs
    ...   Check EDR Has Audit Netlink


EDR Does Not Disable Auditd After Install With Do Not Disable Flag
    ${is_suse} =  Does File Contain Word  /etc/os-release  SUSE Linux Enterprise Server 12
    Pass Execution If  ${is_suse}  LINUXDAR-7106 - test broken on SLES12 - wont be fixed
    Ensure AuditD Running
    Install Base For Component Tests
    Create File  ${SOPHOS_INSTALL}/base/etc/install_options  --do-not-disable-auditd
    Install EDR Directly from SDDS
    Check EDR Plugin Config Contains  disable_auditd=0
    Wait Until Keyword Succeeds
    ...   20 secs
    ...   2 secs
    ...   Check EDR Log Shows AuditD Has Not Been Disabled
    Check AuditD Executable Running


EDR Does Disable Auditd After Manual Change To Config
    ${is_suse} =  Does File Contain Word  /etc/os-release  SUSE Linux Enterprise Server 12
    Pass Execution If  ${is_suse}  LINUXDAR-7106 - test broken on SLES12 - wont be fixed
    Ensure AuditD Running
    Install Base For Component Tests
    Create File  ${SOPHOS_INSTALL}/base/etc/install_options  --do-not-disable-auditd
    Install EDR Directly from SDDS
    Check Install Options File Contains  --do-not-disable-auditd
    Check EDR Plugin Config Contains  disable_auditd=0
    Wait Until Keyword Succeeds
    ...   20 secs
    ...   1 secs
    ...   Check EDR Log Shows AuditD Has Not Been Disabled
    Check AuditD Executable Running
    Run Shell Process  echo disable_auditd=1 > ${EDR_PLUGIN_PATH}/etc/plugin.conf   OnError=failed to set EDR config to disable auditd
    Restart EDR
    Check EDR Plugin Config Contains  disable_auditd=1
    Wait Until Keyword Succeeds
    ...   20 secs
    ...   1 secs
    ...   Check EDR Log Shows AuditD Has Been Disabled
    Check AuditD Executable Not Running


*** Keywords ***
Suite Setup For EDR And Auditd
    Store Whether Auditd Installed
    Store Whether Auditd Is Running
    # If no auitd, install it
    ${auditd_name}=  Get Audit Package Name
    IF    ${auditd_already_installed_when_suite_started} == ${False}
        install_package  ${auditd_name}
    END

Suite Teardown For EDR And Auditd
    # Start up auditd if it was already running when the suite started
    Run Keyword If  ${auditd_already_running_when_suite_started}  Start Auditd
    # Remove auitd if we had to install it for this test suite
    ${auditd_name}=  Get Audit Package Name
    IF    ${auditd_already_installed_when_suite_started} == ${False}
        remove_package  ${auditd_name}
    END
    Uninstall All

Test Teardown For EDR And Auditd
    Common Teardown
    Uninstall All

Store Whether Auditd Installed
    ${auditd_name}=  Get Audit Package Name
    ${auditd_already_installed_when_suite_started}=  is_package_installed  ${auditd_name}
    Set suite variable  ${auditd_already_installed_when_suite_started}

Store Whether Auditd Is Running
    ${status} =   Run Process  systemctl  is-active  auditd
    ${auditd_already_running_when_suite_started}=  Evaluate  ${status.rc} == 0
    Set suite variable  ${auditd_already_running_when_suite_started}

Get Audit Package Name
    # On different distros pkg names differ, on ubuntu it's auditd and on centos and sles it's audit
    ${uses_apt}=  os_uses_apt
    ${audit_pkg_name}=  Set Variable If   ${uses_apt}  auditd  audit
    [Return]  ${audit_pkg_name}

Start Auditd
    Run Shell Process  systemctl unmask auditd  OnError=failed to unmask auditd   timeout=60s
    Run Shell Process  systemctl start auditd   OnError=failed to start auditd   timeout=60s

Check AuditD Executable Running
    ${result} =    Run Process  pgrep  ^auditd
    Should Be Equal As Integers    ${result.rc}    0       msg="stdout:${result.stdout}\nerr: ${result.stderr}"

Check AuditD Executable Not Running
    ${result} =    Run Process  pgrep  ^auditd
    Should Not Be Equal As Integers    ${result.rc}    0     msg="stdout:${result.stdout}\nerr: ${result.stderr}"

Check AuditD Service Disabled
    ${result} =    Run Process  systemctl  is-enabled  auditd
    log  ${result.stdout}
    log  ${result.stderr}
    log  ${result.rc}
    Should Not Be Equal As Integers    ${result.rc}    0

Check EDR Log Shows AuditD Has Been Disabled
    EDR Plugin Log Contains    EDR configuration set to disable AuditD
    EDR Plugin Log Contains    Successfully stopped service: auditd
    EDR Plugin Log Contains    Successfully disabled service: auditd
    EDR Plugin Log Does Not Contain    Failed to mask journald audit socket

Check EDR Log Shows AuditD Has Not Been Disabled
    EDR Plugin Log Does Not Contain    Successfully disabled service: auditd
    EDR Plugin Log Contains    EDR configuration set to not disable AuditD
    EDR Plugin Log Contains    AuditD is running, it will not be possible to obtain event data.

Check EDR Has Audit Netlink
    ${edr_pid} =    Run Process  pgrep -a osquery | grep plugins/edr | grep -v osquery.conf | head -n1 | cut -d " " -f1  shell=true
    ${result} =  Run Process   auditctl -s | grep pid | awk '{print $2}'  shell=True
    Should be equal as strings  ${result.stdout}  ${edr_pid.stdout}

Check EDR Plugin Config Contains
    [Arguments]  ${string_to_contain}
    File Should Contain  ${EDR_PLUGIN_PATH}/etc/plugin.conf  ${string_to_contain}

Check Install Options File Contains
     [Arguments]  ${string_to_contain}
     File Should Contain  ${SOPHOS_INSTALL}/base/etc/install_options  ${string_to_contain}

Ensure AuditD Running
    ${status} =   Run Process  systemctl  is-active  auditd
    Run Keyword If  ${status.rc} != 0   Start Auditd

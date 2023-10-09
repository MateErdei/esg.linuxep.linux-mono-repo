*** Settings ***
Suite Setup      EDR Suite Setup
Suite Teardown   EDR Suite Teardown

Test Setup       Run Keywords
...              EDR Test Setup   AND
...              Run Shell Process    systemctl start auditd   OnError=failed to start auditd   timeout=10s

Test Teardown    EDR Test Teardown


Library     ${LIBS_DIRECTORY}/WarehouseUtils.py
Library     ${LIBS_DIRECTORY}/ThinInstallerUtils.py
Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/MCSRouter.py

Resource    ../upgrade_product/UpgradeResources.robot
Resource    ../GeneralTeardownResource.robot
Resource    EDRResources.robot

Default Tags   THIN_INSTALLER  OSTIA  EDR_PLUGIN  FAKE_CLOUD

*** Variables ***
${BaseAndEdrVUTPolicy}                ${GeneratedWarehousePolicies}/base_and_edr_VUT.xml
${BaseAndEdrAndMtrVUTPolicy}          ${GeneratedWarehousePolicies}/base_edr_and_mtr.xml
${BaseVUTPolicy}                      ${GeneratedWarehousePolicies}/base_only_VUT.xml


*** Test Cases ***
EDR Disables Auditd After Install With Auditd Running Default Behaviour

    Check AuditD Executable Running

    Install EDR  ${BaseAndEdrVUTPolicy}

    ${EDR_CONFIG_CONTENT}=  Get File  ${EDR_DIR}/etc/plugin.conf
    Should Contain  ${EDR_CONFIG_CONTENT}   disable_auditd=1

    Check AuditD Executable Not Running
    Check AuditD Service Disabled
    Check EDR Log Shows AuditD Has Been Disabled

    Wait Until Keyword Succeeds
    ...   20 secs
    ...   2 secs
    ...   Check EDR Osquery Executable Running

    Wait Until Keyword Succeeds
    ...   20 secs
    ...   2 secs
    ...   check edr has audit netlink

    ${contents}=  Get File  ${EDR_DIR}/etc/osquery.flags
    Should contain  ${contents}  --disable_audit=false
    Should contain  ${contents}  --audit_allow_process_events=true
    Should contain  ${contents}  --audit_allow_sockets=true
    Should contain  ${contents}  --audit_allow_user_events=true

EDR Disables Auditd After Install With Auditd Running With Disable Flag
    Check AuditD Executable Running

    Install EDR  ${BaseAndEdrVUTPolicy}  args=--disable-auditd

    ${INSTALL_OPTIONS_CONTENT}=  Get File  ${SOPHOS_INSTALL}/base/etc/install_options
    Should Contain  ${INSTALL_OPTIONS_CONTENT}   --disable-auditd
    ${EDR_CONFIG_CONTENT}=  Get File  ${EDR_DIR}/etc/plugin.conf
    Should Contain  ${EDR_CONFIG_CONTENT}   disable_auditd=1

    Wait Keyword Succeed  Check EDR Log Shows AuditD Has Been Disabled
    Check AuditD Executable Not Running

EDR Does Not Disable Auditd After Install With Auditd Running With Do Not Disable Flag
    Check AuditD Executable Running

    Install EDR  ${BaseAndEdrVUTPolicy}  args=--do-not-disable-auditd


    ${INSTALL_OPTIONS_CONTENT}=  Get File  ${SOPHOS_INSTALL}/base/etc/install_options
    Should Contain  ${INSTALL_OPTIONS_CONTENT}   --do-not-disable-auditd
    ${EDR_CONFIG_CONTENT}=  Get File  ${EDR_DIR}/etc/plugin.conf
    Should Contain  ${EDR_CONFIG_CONTENT}   disable_auditd=0

    Wait Keyword Succeed  Check EDR Log Shows AuditD Has Not Been Disabled
    Check AuditD Executable Running


EDR Does Disable Auditd After Manual Change To Config
    Check AuditD Executable Running

    Install EDR  ${BaseAndEdrVUTPolicy}  args=--do-not-disable-auditd


    ${INSTALL_OPTIONS_CONTENT}=  Get File  ${SOPHOS_INSTALL}/base/etc/install_options
    Should Contain  ${INSTALL_OPTIONS_CONTENT}   --do-not-disable-auditd
    ${EDR_CONFIG_CONTENT}=  Get File  ${EDR_DIR}/etc/plugin.conf
    Should Contain  ${EDR_CONFIG_CONTENT}   disable_auditd=0
    Wait Keyword Succeed  Check EDR Log Shows AuditD Has Not Been Disabled
    Check AuditD Executable Running

    Run Shell Process  echo disable_auditd=1 > ${EDR_DIR}/etc/plugin.conf   OnError=failed to set EDR config to disable auditd
    Run Shell Process  systemctl restart sophos-spl   OnError=failed to restart sophos-spl

    ${EDR_CONFIG_CONTENT}=  Get File  ${EDR_DIR}/etc/plugin.conf
    Should Contain  ${EDR_CONFIG_CONTENT}   disable_auditd=1

    Wait Keyword Succeed  Check EDR Log Shows AuditD Has Been Disabled

    Check AuditD Executable Not Running

EDR Does Disable Auditd When Installed with MTR
    [Tags]   THIN_INSTALLER  OSTIA  EDR_PLUGIN  FAKE_CLOUD  MDR_PLUGIN
    Check AuditD Executable Running
    Install EDR  ${BaseAndEdrAndMtrVUTPolicy}
    Override LogConf File as Global Level  DEBUG
    Restart EDR Plugin
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Should Exist  ${MTR_DIR}

    Check MDR Plugin Installed

    ${EDR_CONFIG_CONTENT}=  Get File  ${EDR_DIR}/etc/plugin.conf
    Should Contain  ${EDR_CONFIG_CONTENT}   disable_auditd=1

    Check AuditD Executable Not Running
    Check AuditD Service Disabled
    Check EDR Log Shows AuditD Has Been Disabled
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  2 secs
    ...  Check EDR Log Contains   Plugin should not gather data from audit subsystem netlink

    Check AuditD Executable Not Running
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check MTR Osquery Executable Running

    Check Log Contains  --disable_audit=true   /opt/sophos-spl/plugins/edr/etc/osquery.flags   osquery.flags

    ${result} =  Check If Process Has Osquery Netlink   /opt/sophos-spl/plugins/edr/bin/osqueryd
    Should Be Equal  ${result}   ${FALSE}

Thin Installer Creates Options File With Many Args

    Start Local Cloud Server  --initial-alc-policy  ${BaseAndEdrVUTPolicy}
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseAndEdrVUTPolicy}  args=--a --b --c

    ${options}=  Get File  ${SOPHOS_INSTALL}/base/etc/install_options
    Should Contain  ${options}  --a
    Should Contain  ${options}  --b
    Should Contain  ${options}  --c

*** Keywords ***
check edr has audit netlink
    ${edr_pid} =    Run Process  pgrep -a osquery | grep plugins/edr | grep -v osquery.conf | head -n1 | cut -d " " -f1  shell=true
    ${result} =  Run Process   auditctl -s | grep pid | awk '{print $2}'  shell=True
    Should be equal as strings  ${result.stdout}  ${edr_pid.stdout}
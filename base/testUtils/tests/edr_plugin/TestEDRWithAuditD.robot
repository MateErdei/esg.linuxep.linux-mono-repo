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


EDR Disables Auditd After Install With Auditd Running With Disable Flag
    Check AuditD Executable Running

    Install EDR  ${BaseAndEdrVUTPolicy}  args=--disable-auditd

    ${INSTALL_OPTIONS_CONTENT}=  Get File  ${SOPHOS_INSTALL}/base/etc/install_options
    Should Contain  ${INSTALL_OPTIONS_CONTENT}   --disable-auditd
    ${EDR_CONFIG_CONTENT}=  Get File  ${EDR_DIR}/etc/plugin.conf
    Should Contain  ${EDR_CONFIG_CONTENT}   disable_auditd=1

    Check EDR Log Shows AuditD Has Been Disabled
    Check AuditD Executable Not Running

EDR Does Not Disable Auditd After Install With Auditd Running With Do Not Disable Flag
    Check AuditD Executable Running

    Install EDR  ${BaseAndEdrVUTPolicy}  args=--do-not-disable-auditd


    ${INSTALL_OPTIONS_CONTENT}=  Get File  ${SOPHOS_INSTALL}/base/etc/install_options
    Should Contain  ${INSTALL_OPTIONS_CONTENT}   --do-not-disable-auditd
    ${EDR_CONFIG_CONTENT}=  Get File  ${EDR_DIR}/etc/plugin.conf
    Should Contain  ${EDR_CONFIG_CONTENT}   disable_auditd=0

    Check EDR Log Shows AuditD Has Not Been Disabled
    Check AuditD Executable Running


EDR Does Disable Auditd After Manual Change To Config
    Check AuditD Executable Running

    Install EDR  ${BaseAndEdrVUTPolicy}  args=--do-not-disable-auditd


    ${INSTALL_OPTIONS_CONTENT}=  Get File  ${SOPHOS_INSTALL}/base/etc/install_options
    Should Contain  ${INSTALL_OPTIONS_CONTENT}   --do-not-disable-auditd
    ${EDR_CONFIG_CONTENT}=  Get File  ${EDR_DIR}/etc/plugin.conf
    Should Contain  ${EDR_CONFIG_CONTENT}   disable_auditd=0
    Check EDR Log Shows AuditD Has Not Been Disabled
    Check AuditD Executable Running

    Run Shell Process  echo disable_auditd=1 > ${EDR_DIR}/etc/plugin.conf   OnError=failed to set EDR config to disable auditd
    Run Shell Process  systemctl restart sophos-spl   OnError=failed to restart sophos-spl

    ${EDR_CONFIG_CONTENT}=  Get File  ${EDR_DIR}/etc/plugin.conf
    Should Contain  ${EDR_CONFIG_CONTENT}   disable_auditd=1

     Wait Until Keyword Succeeds
    ...   10 secs
    ...   1 secs
    ...   Check EDR Log Shows AuditD Has Been Disabled

    Check AuditD Executable Not Running

Thin Installer Creates Options File With Many Args

    Start Local Cloud Server  --initial-alc-policy  ${BaseAndEdrVUTPolicy}
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseAndEdrVUTPolicy}  args=--a --b --c

    ${options}=  Get File  ${SOPHOS_INSTALL}/base/etc/install_options
    Should Contain  ${options}  --a
    Should Contain  ${options}  --b
    Should Contain  ${options}  --c
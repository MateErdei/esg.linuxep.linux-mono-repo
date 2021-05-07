*** Settings ***
Suite Setup      EDR Suite Setup
Suite Teardown   EDR Suite Teardown

Test Setup       Run Keywords
...              EDR Test Setup   AND
...              Ensure AuditD Running

Test Teardown    EDR Test Teardown


Library     ${LIBS_DIRECTORY}/WarehouseUtils.py
Library     ${LIBS_DIRECTORY}/ThinInstallerUtils.py
Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/MCSRouter.py

Resource    ../upgrade_product/UpgradeResources.robot
Resource    ../GeneralTeardownResource.robot
Resource    EDRResources.robot

Default Tags   THIN_INSTALLER  OSTIA  EDR_PLUGIN  FAKE_CLOUD
Force Tags  LOAD2

*** Variables ***
${BaseAndEdrVUTPolicy}                ${GeneratedWarehousePolicies}/base_and_edr_VUT.xml
${BaseAndEdrAndMtrVUTPolicy}          ${GeneratedWarehousePolicies}/base_edr_and_mtr.xml
${BaseVUTPolicy}                      ${GeneratedWarehousePolicies}/base_only_VUT.xml


*** Test Cases ***
Thin Installer Creates Options File With Many Args

    Start Local Cloud Server  --initial-alc-policy  ${BaseAndEdrVUTPolicy}
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseAndEdrVUTPolicy}  args=--a --b --c

    ${options}=  Get File  ${SOPHOS_INSTALL}/base/etc/install_options
    Should Contain  ${options}  --a
    Should Contain  ${options}  --b
    Should Contain  ${options}  --c

*** Keywords ***
Check EDR Has Audit Netlink
    ${edr_pid} =    Run Process  pgrep -a osquery | grep plugins/edr | grep -v osquery.conf | head -n1 | cut -d " " -f1  shell=true
    ${result} =  Run Process   auditctl -s | grep pid | awk '{print $2}'  shell=True
    Should be equal as strings  ${result.stdout}  ${edr_pid.stdout}

Ensure AuditD Running
    ${status} =   Run Process  systemctl  is-active  auditd
    Run Keyword Unless  '${status.stdout}' == 'active' or '${status.stdout}' == 'inactive'  Sleep  10s
    Run Keyword If  ${status.rc} != 0  Run Shell Process  systemctl start auditd   OnError=failed to start auditd   timeout=60s

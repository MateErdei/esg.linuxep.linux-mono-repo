*** Settings ***
Library     ../libs/Cleanup.py
Library     ../libs/HostsUtils.py
Library     ../libs/OSUtils.py
Library     ../libs/ThinInstallerUtils.py
Library     ../libs/UpdateServer.py
Library     Process
Library     OperatingSystem

Resource  FakeCloud.robot

*** Variables ***
${CUSTOM_DIR_BASE} =  /CustomPath
${INPUT_DIRECTORY} =  /opt/test/inputs
${CUSTOMER_DIRECTORY} =  ${INPUT_DIRECTORY}/customer
${WAREHOUSE_DIRECTORY} =  ${INPUT_DIRECTORY}/warehouse
${THIN_INSTALLER_DIRECTORY} =  ${INPUT_DIRECTORY}/thin_installer
${UPDATE_CREDENTIALS} =  7ca577258eb34a6b2a1b04b3e817b76a
${SOPHOS_INSTALL} =   /opt/sophos-spl

*** Keywords ***
Start Warehouse servers
    [Arguments]    ${customer_file_protocol}=--tls1_2   ${warehouse_protocol}=--tls1_2
    Start Update Server    1233    ${CUSTOMER_DIRECTORY}/   ${customer_file_protocol}
    Start Update Server    443    ${WAREHOUSE_DIRECTORY}/   ${warehouse_protocol}   --strip=/dev/sspl-warehouse/develop/warehouse/warehouse/
    Sleep  0.1
    Register Cleanup  Stop Update Servers


Base Test Setup
    Uninstall SSPL if installed


Teardown
    Run Teardown Functions

Install Local SSL Server Cert To System
    Copy File   ${SUPPORT_FILES}/https/ca/root-ca.crt.pem    ${SUPPORT_FILES}/https/ca/root-ca.crt
    Install System Ca Cert  ${SUPPORT_FILES}/https/ca/root-ca.crt
    Register Cleanup  Cleanup System Ca Certs

Setup Hostsfile
    Register Cleanup  revert hostfile
    append host file  127.0.0.1  d1.sophosupd.com d1.sophosupd.net ostia.eng.sophos

Setup Ostia Warehouse Environment
    Generate Local Ssl Certs If They Dont Exist
    Install Local SSL Server Cert To System
    Setup Hostsfile

Setup Warehouses
    Start Warehouse servers
    Setup Ostia Warehouse Environment



Create Thin Installer
    [Arguments]  ${MCS_URL}=https://dzr-mcs-amzn-eu-west-1-f9b7.upe.d.hmr.sophos.com/sophos/management/ep
    extract thin installer  ${THIN_INSTALLER_DIRECTORY}  /tmp/SophosInstallBase.sh
    create credentials file  /tmp/credentials.txt
    ...   b370c75f6dd86503c8cca4edbbd29b5b06162fa9b4e67f992066120ee22612d6
    ...   ${MCS_URL}
    ...   ${UPDATE_CREDENTIALS}

    build thininstaller from sections  /tmp/credentials.txt  /tmp/SophosInstallBase.sh  /tmp/SophosInstallCombined.sh


Check Base Installed
    Directory Should Exist  ${SOPHOS_INSTALL}
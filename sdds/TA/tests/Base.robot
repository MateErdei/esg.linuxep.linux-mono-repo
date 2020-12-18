*** Settings ***
Documentation   Test using thin installer to install base

Library     ../libs/Cleanup.py
Library     ../libs/OSUtils.py
Library     ../libs/ThinInstallerUtils.py
Library     ../libs/UpdateServer.py
Library     Process
Library     OperatingSystem

Resource  FakeCloud.robot

Force Tags  WAREHOUSE  BASE

Test Setup      Base Test Setup
Test Teardown   Teardown

*** Variables ***
${CUSTOM_DIR_BASE} =  /CustomPath
${INPUT_DIRECTORY} =  /opt/test/inputs
${CUSTOMER_DIRECTORY} =  ${INPUT_DIRECTORY}/customer
${WAREHOUSE_DIRECTORY} =  ${INPUT_DIRECTORY}/warehouse
${THIN_INSTALLER_DIRECTORY} =  ${INPUT_DIRECTORY}/thin_installer
${UPDATE_CREDENTIALS} =  7ca577258eb34a6b2a1b04b3e817b76a

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

Setup Ostia Warehouse Environment
    Generate Local Ssl Certs If They Dont Exist
    Install Local SSL Server Cert To System

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


*** Test Case ***
Thin Installer can install Base to Dev Region
    Setup Warehouses
    Create Thin Installer
    Run Thin Installer  /tmp/SophosInstallCombined.sh  ${0}  http://localhost:1233  ${SUPPORT_FILES}/certs/hmr-dev-sha1.pem

Thin Installer can install Base to Local Fake Cloud
    [Tags]  FAIL
    Setup Warehouses
    Create Thin Installer  https://localhost:9999/
    Start Fake Cloud
    Run Thin Installer  /tmp/SophosInstallCombined.sh  ${0}  http://localhost:1233  ${SUPPORT_FILES}/certs/hmr-dev-sha1.pem

*** Settings ***
Documentation   Test using thin installer to install base

Library     ../libs/OnFail.py
Library     ../libs/OSUtils.py
Library     ../libs/ThinInstallerUtils.py
Library     ../libs/UpdateServer.py
Library     Process
Library     OperatingSystem

Force Tags  WAREHOUSE  BASE

Test Teardown   Teardown

*** Variables ***
${CUSTOM_DIR_BASE} =  /CustomPath
${INPUT_DIRECTORY} =  /opt/test/inputs
${CUSTOMER_DIRECTORY} =  ${INPUT_DIRECTORY}/customer
${WAREHOUSE_DIRECTORY} =  ${INPUT_DIRECTORY}/warehouse
${THIN_INSTALLER_DIRECTORY} =  ${INPUT_DIRECTORY}/thin_installer
${UPDATE_CREDENTIALS} =  foobar

*** Keywords ***
Start Warehouse servers
    [Arguments]    ${customer_file_protocol}=--tls1_2   ${warehouse_protocol}=--tls1_2
    Start Update Server    1233    ${CUSTOMER_DIRECTORY}/   ${customer_file_protocol}
    Start Update Server    1234    ${WAREHOUSE_DIRECTORY}/   ${warehouse_protocol}
    Sleep  1
    Register Cleanup  Stop Update Servers

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


Run ThinInstaller
    [Arguments]  ${expected_return}  ${update_url}
    extract thin installer  ${THIN_INSTALLER_DIRECTORY}  /tmp/SophosInstallBase.sh
    create credentials file  /tmp/credentials.txt
    ...   b370c75f6dd86503c8cca4edbbd29b5b06162fa9b4e67f992066120ee22612d6
    ...   https://dzr-mcs-amzn-eu-west-1-f9b7.upe.d.hmr.sophos.com/sophos/management/ep
    ...   ${UPDATE_CREDENTIALS}

    build thininstaller from sections  /tmp/credentials.txt  /tmp/SophosInstallBase.sh  /tmp/SophosInstallCombined.sh


*** Test Case ***
Thin Installer can install Base
    Setup Warehouses
    Run Thininstaller    0    http://localhost:1233

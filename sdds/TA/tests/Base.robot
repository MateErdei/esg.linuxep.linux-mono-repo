*** Settings ***
Documentation   Test using thin installer to install base

Library     ../libs/OnFail.py
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

*** Keywords ***
Start Warehouse servers
    [Arguments]    ${customer_file_protocol}=--tls1_2   ${warehouse_protocol}=--tls1_2
    Start Update Server    1233    ${CUSTOMER_DIRECTORY}/   ${customer_file_protocol}
    Start Update Server    1234    ${WAREHOUSE_DIRECTORY}/   ${warehouse_protocol}
    Sleep  1
    Register Cleanup  Stop Update Servers

Teardown
    Run Teardown Functions

*** Test Case ***
Thin Installer can install Base
    Start Warehouse servers
#    Run Default Thininstaller    0    http://localhost:1233  force_certs_dir=${SUPPORT_FILES}/sophos_certs

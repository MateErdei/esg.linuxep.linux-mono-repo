
*** Settings ***
Documentation   Component tests for SOAP
Force Tags      COMPONENT  PRODUCT  SOAP  onaccess  oa_alternative

Library    OperatingSystem
Library    ../Libs/OnFail.py

Resource  ../shared/OnAccessResources.robot


Test Teardown  On Access Test Teardown

*** Test Cases ***

soapd handles missing log directory
    [Tags]  fault_injection
    # Plugin and Base already mock installed by __init__.robot
    Remove Log directory

    register cleanup  dump logs  /tmp/soapd.stdout  /tmp/soapd.stderr  ${ON_ACCESS_LOG_PATH}

    Start On Access without Log check

    # soapd will re-create the logging directory
    Wait Until Created  ${ON_ACCESS_LOG_PATH}  timeout=10s
    Directory Should Exist  ${AV_PLUGIN_PATH}/log

soapd handles missing threat detector socket
    [Tags]  fault_injection
    register cleanup  dump logs  /tmp/soapd.stdout  /tmp/soapd.stderr  ${ON_ACCESS_LOG_PATH}

    # copy the flags
    Copy File  ${RESOURCES_PATH}/oa_config/flags_on.json  ${COMPONENT_VAR_DIR}/oa_flag.json
    # copy config
    Copy File  ${RESOURCES_PATH}/oa_config/config_on.json  ${COMPONENT_VAR_DIR}/soapd_config.json

    Start On Access
    Sleep  ${1}

    ${oa_mark} =  get_on_access_log_mark
    On-access Scan Clean File

    wait for on access log contains after mark  OnAccessImpl <> Failed to connect to Sophos Threat Detector - retrying after sleep
    ...  mark=${oa_mark}  timeout=${5}


*** Variables ***


*** Keywords ***

Remove Log directory
    Remove Directory  ${AV_PLUGIN_PATH}/log  recursive=${True}
    Register Cleanup  Create Directory  ${AV_PLUGIN_PATH}/log


On Access Test Teardown
    run teardown functions
    Terminate All Processes

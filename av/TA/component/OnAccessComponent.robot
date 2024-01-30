
*** Settings ***
Documentation   Component tests for SOAP
Force Tags      COMPONENT  PRODUCT  SOAP  onaccess  oa_alternative  TAP_PARALLEL1

Library    OperatingSystem
Library    ${COMMON_TEST_LIBS}/OnFail.py

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
    Copy File  ${RESOURCES_PATH}/oa_config/config_on.json  ${COMPONENT_VAR_DIR}/on_access_policy.json

    Start On Access
    Sleep  ${1}

    ${oa_mark} =  get_on_access_log_mark
    On-access Scan Clean File

    wait for on access log contains after mark  OnAccessImpl <> Failed to connect to Sophos Threat Detector - retrying after sleep
    ...  mark=${oa_mark}  timeout=${5}

soapd handles missing var directory
    [Tags]  fault_injection
    register cleanup  dump logs  /tmp/soapd.stdout  /tmp/soapd.stderr  ${ON_ACCESS_LOG_PATH}

    Remove Var directory
    ${oa_mark} =  get_on_access_log_mark
    Start On Access without Pid check

    wait for on access log contains after mark  Exception caught at top-level: Unable to open lock file /opt/sophos-spl/plugins/av/var/soapd.pid because No such file or directory(2)
        ...  mark=${oa_mark}  timeout=${5}

soapd handles process control socket already exists as a directory
    [Tags]  fault_injection
    Create Directory    ${COMPONENT_VAR_DIR}/soapd_controller
    Register Cleanup  Remove Directory  ${COMPONENT_VAR_DIR}/soapd_controller

    ${oa_mark} =  get_on_access_log_mark
    Start On Access without Pid check

    wait for on access log contains after mark   Exception caught at top-level: ProcessControlServer failed to bind to unix socket path
            ...  mark=${oa_mark}  timeout=${5}

*** Variables ***


*** Keywords ***

Remove Log directory
    Remove Directory  ${AV_PLUGIN_PATH}/log  recursive=${True}
    Register Cleanup  Restore Log directory

Restore Log directory
    Create Directory  ${AV_PLUGIN_PATH}/log
    Run Process   chmod  a+rwx
    ...  ${COMPONENT_ROOT_PATH}/log  ${COMPONENT_ROOT_PATH}/chroot/log
    Run Process   ln  -snf  ${COMPONENT_ROOT_PATH}/chroot/log  ${COMPONENT_ROOT_PATH}/log/sophos_threat_detector

Remove Var directory
    Remove Directory  ${AV_PLUGIN_PATH}/var  recursive=${True}
    Register Cleanup  Restore Var directory

Restore Var directory
    Create Directory  ${AV_PLUGIN_PATH}/var
    Run Process   chmod  a+rwx  ${COMPONENT_ROOT_PATH}/var

On Access Test Teardown
    run teardown functions
    Terminate All Processes

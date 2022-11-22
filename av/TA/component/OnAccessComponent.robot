
*** Settings ***
Documentation   Component tests for SOAP
Force Tags      COMPONENT  PRODUCT  SOAP  onaccess  oa_alternative

Library    OperatingSystem
Library    ../Libs/OnFail.py

Resource  ../shared/OnAccessResources.robot


Test Teardown  On Access Test Teardown

*** Test Cases ***

soapd handles missing log directory
    # Plugin and Base already mock installed by __init__.robot
    Remove Directory  ${AV_PLUGIN_PATH}/log  recursive=${True}
    Register Cleanup  Create Directory  ${AV_PLUGIN_PATH}/log

    register cleanup  dump logs  /tmp/soapd.stdout  /tmp/soapd.stderr  ${ON_ACCESS_LOG_PATH}

    Start On Access without Log check

    # soapd will re-create the logging directory
    Wait Until Created  ${ON_ACCESS_LOG_PATH}  timeout=10s
    Directory Should Exist  ${AV_PLUGIN_PATH}/log

*** Variables ***


*** Keywords ***

On Access Test Teardown
    run teardown functions
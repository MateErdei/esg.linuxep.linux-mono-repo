*** Settings ***
Library         Process
Library         OperatingSystem
Library         ../Libs/FakeManagement.py
Library         ${COMMON_TEST_LIBS}/LogUtils.py
Library         ../Libs/UserUtils.py
Library         ../Libs/fixtures/EDRPlugin.py

Resource    ComponentSetup.robot
Resource    EDRResources.robot

Suite Setup  Setup Base And Component
Suite Teardown  Run Keywords
...             Uninstall EDR  AND
...             Remove Directory   ${SOPHOS_INSTALL}   recursive=True

Test Setup      Component Test Setup
Test Teardown   Component Test TearDown

Force Tags    TAP_PARALLEL4

*** Variables ***
${EDR_PLUGIN_PATH}  ${COMPONENT_ROOT_PATH}
${EDR_PLUGIN_BIN}   ${COMPONENT_BIN_PATH}
${EDR_LOG_PATH}    ${EDR_PLUGIN_PATH}/log/edr.log

*** Test Cases ***
EDR Plugin Can Receive Actions
    create_users_and_group
    ${handle} =  Start Process  ${EDR_PLUGIN_BIN}

    Check EDR Plugin Installed

    ${actionContent} =  Set Variable  This is an action test
    
    ${mark} =  Mark Log Size    ${EDR_LOG_PATH}
    Send Plugin Action  edr  LiveQuery  corr123  ${actionContent}
    Wait For Log Contains From Mark    ${mark}    Received new Action  ${15}

EDR plugin Can Send Status
    create_users_and_group
    ${handle} =  Start Process  ${EDR_PLUGIN_BIN}
    Check EDR Plugin Installed

    ${edrStatus}=  Get Plugin Status  edr  LiveQuery
    Should Contain  ${edrStatus}   RevID

    ${edrTelemetry}=  Get Plugin Telemetry  edr
    Should Contain  ${edrTelemetry}   osquery-restarts

    ${result} =   Terminate Process  ${handle}

EDR Plugin Does Not Hang When OSQuery Socket Is Not Created
    [Documentation]  This test will replace osquery with a simple bash script to ensure that EDR does not hang
    ...  if OSQUery fails to start-up properly.
    [Teardown]  EDR Hanging TearDown

    create_users_and_group

    Move File  ${EDR_PLUGIN_PATH}/bin/osqueryd  ${EDR_PLUGIN_PATH}/bin/osqueryd.bak
    Create File   ${EDR_PLUGIN_PATH}/bin/osqueryd   echo starting
    Run Process  chmod  +x  ${EDR_PLUGIN_PATH}/bin/osqueryd

    ${handle} =  Start Process  ${EDR_PLUGIN_BIN}
    Check EDR Plugin Installed

    ${ExpectedLogMessages}=  Create List
            ...  Run osquery process
            ...  Timed out waiting for osquery socket file to be created
            ...  The osquery process finished
            ...  Restarting osquery

    # Check edr.log file to ensure EDR will retry to restart OSQUery if OSQuery has not started correctly.
    Wait Until Keyword Succeeds
    ...   200 secs
    ...   5 secs
    ...   Log Contains In Order   ${EDR_PLUGIN_PATH}/log/edr.log   edr.log  ${ExpectedLogMessages}

EDR Plugin Purges OSQuery Database If Event Data Retention Time Check Fails
    [Documentation]  This test will replace osquery with a simple bash script to ensure that EDR does not hang
    ...  if OSQUery fails to start-up properly.
    [Teardown]  EDR Hanging TearDown

    create_users_and_group

    Move File  ${EDR_PLUGIN_PATH}/bin/osqueryd  ${EDR_PLUGIN_PATH}/bin/osqueryd.bak
    Create File   ${EDR_PLUGIN_PATH}/bin/osqueryd   echo starting
    Create Directory   ${EDR_PLUGIN_PATH}/etc/osquery.conf.d/
    Create File  ${EDR_PLUGIN_PATH}/var/osquery.db/test_file   echo basic_test_file_to_prove_purge_works
    Run Process  chmod  +x  ${EDR_PLUGIN_PATH}/bin/osqueryd

    ${handle} =  Start Process  ${EDR_PLUGIN_BIN}
    Check EDR Plugin Installed

    ${ExpectedLogMessages}=  Create List
            ...  Run osquery process
            ...  Timed out waiting for osquery socket file to be created
            ...  The osquery process finished
            ...  Restarting osquery

    Wait Until Removed  ${EDR_PLUGIN_PATH}/var/osquery.db/test_file  200 secs


EDR Plugin Will Create OSQuery Options File To Configure OSQuery Data Retention Time
    create_users_and_group
    ${handle} =  Start Process  ${EDR_PLUGIN_BIN}
    Create Directory   ${EDR_PLUGIN_PATH}/etc/osquery.conf.d/
    Check EDR Plugin Installed
    Wait Until Created   ${EDR_PLUGIN_PATH}/etc/osquery.conf.d/options.conf  200 secs

    Log File  ${EDR_PLUGIN_PATH}/etc/osquery.conf.d/options.conf


*** Keywords ***

EDR Hanging TearDown
    Remove File   ${EDR_PLUGIN_PATH}/bin/osqueryd
    Move File  ${EDR_PLUGIN_PATH}/bin/osqueryd.bak  ${EDR_PLUGIN_PATH}/bin/osqueryd
    Component Test TearDown
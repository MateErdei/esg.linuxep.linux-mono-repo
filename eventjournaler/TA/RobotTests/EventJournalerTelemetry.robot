*** Settings ***
Documentation   Suite description

Library         Collections

Resource        EventJournalerResources.robot
Library         ../Libs/InstallerUtils.py
Library         ${TEST_INPUT_PATH}/fake_management/FakeManagement.py

Suite Setup     Setup
Suite Teardown  Uninstall Base

Test Teardown  Test Teardown


*** Test Cases ***
Check Health Telemetry Is Written
    Publish Threat Event With Specific Data  {"threatName":"EICAR-AV-Test","test data 1"}

    ${EJr_telemetry} =  Get Plugin Telemetry  eventjournaler
    Log  ${EJr_telemetry}
    ${telemetry_dict} =  Evaluate  json.loads('''${EJr_telemetry}''')  json

    Dictionary Should Contain Key  ${telemetry_dict}  acceptable-daily-dropped-events-exceeded
    Dictionary Should Contain Key  ${telemetry_dict}  event-subscriber-socket-missing
    Dictionary Should Contain Key  ${telemetry_dict}  health
    Dictionary Should Contain Key  ${telemetry_dict}  thread-health
    Dictionary Should Contain Key  ${telemetry_dict}  attempted-journal-writes
    ${threadHealthDict} =  Set Variable  ${telemetry_dict['thread-health']}
    Dictionary Should Contain Key  ${threadHealthDict}  PluginAdapter
    Dictionary Should Contain Key  ${threadHealthDict}  Subscriber
    Dictionary Should Contain Key  ${threadHealthDict}  Writer


*** Keywords ***
Test Teardown
    Event Journaler Teardown
    Uninstall Event Journaler
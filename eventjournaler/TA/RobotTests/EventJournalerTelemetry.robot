*** Settings ***
Documentation   Suite description

Library         Collections
Library         OperatingSystem

Resource        EventJournalerResources.robot
Library         ../Libs/InstallerUtils.py
Library         ${COMMON_TEST_LIBS}/FakeManagement.py

Suite Teardown  Uninstall Base

Test Setup     Setup
Test Teardown  Test Teardown

Default Tags    TAP_PARALLEL3

*** Test Cases ***
Check Health Telemetry Is Written
    Publish Threat Event With Specific Data  {"threatName":"EICAR-AV-Test","test data 1"}

    ${EJr_telemetry} =  Get Plugin Telemetry  eventjournaler
    Log  ${EJr_telemetry}
    ${telemetry_dict} =  Evaluate  json.loads('''${EJr_telemetry}''')  json

    Dictionary Should Contain Key  ${telemetry_dict}  acceptable-daily-dropped-events-exceeded
    Should Be False  ${telemetry_dict["acceptable-daily-dropped-events-exceeded"]}
    Dictionary Should Contain Key  ${telemetry_dict}  event-subscriber-socket-missing
    Should Be False  ${telemetry_dict["event-subscriber-socket-missing"]}
    Dictionary Should Contain Key  ${telemetry_dict}  health
    Should Be Equal As Integers  ${telemetry_dict["health"]}  0
    Dictionary Should Contain Key  ${telemetry_dict}  thread-health
    Should Be True  ${telemetry_dict["thread-health"]}
    Dictionary Should Contain Key  ${telemetry_dict}  attempted-journal-writes
    Should Be Equal As Integers  ${telemetry_dict["attempted-journal-writes"]}  1
    ${threadHealthDict} =  Set Variable  ${telemetry_dict['thread-health']}
    Dictionary Should Contain Key  ${threadHealthDict}  PluginAdapter
    Should Be True  ${threadHealthDict["PluginAdapter"]}
    Dictionary Should Contain Key  ${threadHealthDict}  Subscriber
    Should Be True  ${threadHealthDict["Subscriber"]}
    Dictionary Should Contain Key  ${threadHealthDict}  Writer
    Should Be True  ${threadHealthDict["Writer"]}

Check Socket Missing Is True When Socket Deleted
    Wait Until Created  ${SOPHOS_INSTALL}/var/ipc/events.ipc

    ${EJr_telemetry} =  Get Plugin Telemetry  eventjournaler
    Log  ${EJr_telemetry}
    ${telemetry_dict} =  Evaluate  json.loads('''${EJr_telemetry}''')  json
    Dictionary Should Contain Key  ${telemetry_dict}  event-subscriber-socket-missing
    Should Be False  ${telemetry_dict["event-subscriber-socket-missing"]}

    Remove Subscriber Socket

    ${EJr_telemetry} =  Get Plugin Telemetry  eventjournaler
    Log  ${EJr_telemetry}
    ${telemetry_dict} =  Evaluate  json.loads('''${EJr_telemetry}''')  json
    Dictionary Should Contain Key  ${telemetry_dict}  event-subscriber-socket-missing
    Should Be True  ${telemetry_dict["event-subscriber-socket-missing"]}


Check Health Is Good After A Period of Time
    # Health should still be good 1 minute after install.
    sleep    60
    ${telemetry} =  Get Plugin Telemetry  eventjournaler
    Log  ${telemetry}
    ${telemetry_dict} =  Evaluate  json.loads('''${telemetry}''')  json

    Dictionary Should Contain Key  ${telemetry_dict}  event-subscriber-socket-missing
    Should Be False  ${telemetry_dict["event-subscriber-socket-missing"]}

    Dictionary Should Contain Key  ${telemetry_dict}  health
    Should Be Equal As Integers  ${telemetry_dict["health"]}  0

    Dictionary Should Contain Key  ${telemetry_dict}  thread-health
    Should Be True  ${telemetry_dict["thread-health"]}

    ${threadHealthDict} =  Set Variable  ${telemetry_dict['thread-health']}
    Dictionary Should Contain Key  ${threadHealthDict}  PluginAdapter
    Should Be True  ${threadHealthDict["PluginAdapter"]}

    Dictionary Should Contain Key  ${threadHealthDict}  Subscriber
    Should Be True  ${threadHealthDict["Subscriber"]}

    Dictionary Should Contain Key  ${threadHealthDict}  Writer
    Should Be True  ${threadHealthDict["Writer"]}


*** Keywords ***
Test Teardown
    Event Journaler Teardown
    Uninstall Event Journaler

Should Be False
    [Arguments]  ${value}
    Should Be True  ${value} == ${False}
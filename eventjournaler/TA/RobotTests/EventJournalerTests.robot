*** Settings ***
Resource  EventJournalerResources.robot
Suite Setup  Setup
Test Teardown   Event Journaler Teardown
Suite Teardown  Uninstall Base

*** Test Cases ***
Event Journaler Can Receive Events
    ${mark} =  Mark File  ${EVENT_JOURNALER_LOG_PATH}
    Publish Threat Event
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  5 secs
    ...  Check Marked Event Journaler Log contains Contains  {"threatName":"EICAR-AV-Test","threatPath":"/home/admin/eicar.com"}   ${mark}


Event Journaler Can Receive Events From Multiple Publishers
    [Teardown]  Custom Teardown For Tests With Publishers Running In Background
    ${mark} =  Mark File  ${EVENT_JOURNALER_LOG_PATH}
    Publish Threat Events In Background Process   "msg-from-pub1"  100
    Publish Threat Events In Background Process   "msg-from-pub2"  100
    Publish Threat Events In Background Process   "msg-from-pub3"  100

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Marked Event Journaler Log contains Contains  msg-from-pub1  ${mark}

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Marked Event Journaler Log contains Contains  msg-from-pub2  ${mark}

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Marked Event Journaler Log contains Contains  msg-from-pub2  ${mark}


Event Journaler Can Receive Many Events From Publisher
    [Teardown]  Custom Teardown For Tests With Publishers Running In Background
    ${mark} =  Mark File  ${EVENT_JOURNALER_LOG_PATH}
    Publish Threat Events In Background Process   "msg-from-pub1"  100
    Wait Until Marked Journaler Log Contains String X Times  msg-from-pub1  100  ${mark}


Event Journaler Can Be Stopped And Started
    Restart Event Journaler


*** Keywords ***
Setup
    Install Base For Component Tests
    Install Event Journaler Directly from SDDS
    Run Shell Process  chmod a+x ${EVENT_PUB_SUB_TOOL}  OnError=Failed to chmod EventPubSub binary   timeout=3s

Custom Teardown For Tests With Publishers Running In Background
    Event Journaler Teardown
    Terminate All Processes

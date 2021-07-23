*** Settings ***
Resource  EventJournalerResources.robot
Suite Setup  Setup
Test Teardown   Event Journaler Teardown
Suite Teardown  Uninstall Base

*** Test Cases ***
Event Journaler Can Receive Events
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Should Exist   ${EVENT_JOURNALER_LOG_PATH}
    ${mark} =  Mark File  ${EVENT_JOURNALER_LOG_PATH}

    Publish Threat Event

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check Journal Contains Detection Event With Content   {"threatName":"EICAR-AV-Test","threatPath":"/home/admin/eicar.com"}

Event Journaler Restarts Subscriber After Socket Removed
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Should Exist   ${EVENT_JOURNALER_LOG_PATH}
    ${mark} =  Mark File  ${EVENT_JOURNALER_LOG_PATH}

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Should Exist  ${SOPHOS_INSTALL}/var/ipc/events.ipc

    # Delete socket file, subscriber should exit and Journaler should restart it.
    Remove Subscriber Socket

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Marked Event Journaler Log Contains  The subscriber socket has been unexpectedly removed   ${mark}

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Marked Event Journaler Log Contains  Subscriber not running, restarting it   ${mark}

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Marked Event Journaler Log Contains  Subscriber restart called   ${mark}


Event Journaler Can Receive Events From Multiple Publishers
    [Teardown]  Custom Teardown For Tests With Publishers Running In Background
    ${mark} =  Mark File  ${EVENT_JOURNALER_LOG_PATH}
    Publish Threat Events In Background Process   {"threatName":"EICAR-AV-Test","test data 1"}  1
    Publish Threat Events In Background Process   {"threatName":"EICAR-AV-Test","test data 2"}  1
    Publish Threat Events In Background Process   {"threatName":"EICAR-AV-Test","test data 3"}  1

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Get Three Events For Multiple Publishers Test


Event Journaler Can Receive Many Events From Publisher
    FOR  ${i}  IN RANGE  100
        Publish Threat Event With Specific Data  {"threatName":"EICAR-AV-Test","test data":"${i}"}
    END

    ${all_events} =  Read All Detection Events From Journal
    FOR  ${i}  IN RANGE  100
        should contain   ${all_events}  {"threatName":"EICAR-AV-Test","test data":"${i}"}
    END


Event Journaler Can Be Stopped And Started
    Restart Event Journaler


*** Keywords ***
Setup
    Install Base For Component Tests
    Install Event Journaler Directly from SDDS
    Run Shell Process  chmod a+x ${EVENT_PUB_SUB_TOOL}  OnError=Failed to chmod EventPubSub binary tool   timeout=3s
    Run Shell Process  chmod a+x ${EVENT_READER_TOOL}  OnError=Failed to chmod JournalReader binary tool   timeout=3s

Custom Teardown For Tests With Publishers Running In Background
    Event Journaler Teardown
    Terminate All Processes

Get Three Events For Multiple Publishers Test
    ${all_events} =  Read All Detection Events From Journal
    should contain   ${all_events}  {"threatName":"EICAR-AV-Test","test data 1"}
    should contain   ${all_events}  {"threatName":"EICAR-AV-Test","test data 2"}
    should contain   ${all_events}  {"threatName":"EICAR-AV-Test","test data 3"}
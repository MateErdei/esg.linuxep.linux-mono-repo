*** Settings ***
Resource  EventJournalerResources.robot
Suite Setup  Setup
Test Teardown   Event Journaler Teardown
Suite Teardown  Uninstall Base

*** Test Cases ***
Event Journaler Can Receive Events
    ${mark} =  Mark File  ${EVENT_JOURNALER_LOG_PATH}
    Run Shell Process  chmod a+x ${EVENT_PUB_SUB_TOOL}  OnError=Failed to chmod EventPubSub binary   timeout=3s
    Run Shell Process  ${EVENT_PUB_SUB_TOOL} -s /opt/sophos-spl/var/ipc/events.ipc send  OnError=Failed to run EventPubSub binary   timeout=60s

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  5 secs
    ...  Check Marked Event Journaler Log contains Contains  {"threatName":"EICAR-AV-Test","threatPath":"/home/admin/eicar.com"}   ${mark}


Event Journaler Can Receive Events From Multiple Publishers
    [Teardown]  Custom Teardown For Multi Pub Test
    ${mark} =  Mark File  ${EVENT_JOURNALER_LOG_PATH}
    Run Shell Process  chmod a+x ${EVENT_PUB_SUB_TOOL}  OnError=Failed to chmod EventPubSub binary   timeout=3s
    Start Process 	timeout 30s ${EVENT_PUB_SUB_TOOL} -s /opt/sophos-spl/var/ipc/events.ipc send -d "msg-from-pub1" -c 100  alias=publisher1  shell=True
    Start Process 	timeout 30s ${EVENT_PUB_SUB_TOOL} -s /opt/sophos-spl/var/ipc/events.ipc send -d "msg-from-pub2" -c 100  alias=publisher2  shell=True
    Start Process 	timeout 30s ${EVENT_PUB_SUB_TOOL} -s /opt/sophos-spl/var/ipc/events.ipc send -d "msg-from-pub3" -c 100  alias=publisher3  shell=True

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
    [Teardown]  Custom Teardown For Multi Pub Test
    ${mark} =  Mark File  ${EVENT_JOURNALER_LOG_PATH}
    Run Shell Process  chmod a+x ${EVENT_PUB_SUB_TOOL}  OnError=Failed to chmod EventPubSub binary   timeout=3s
    Start Process 	timeout 30s ${EVENT_PUB_SUB_TOOL} -s /opt/sophos-spl/var/ipc/events.ipc send -d "msg-from-pub1" -c 100  alias=publisher1  shell=True
    Wait Until Marked Journaler Log Contains String X Times  msg-from-pub1  100  ${mark}


Event Journaler Can be stopped and started
    Restart Event Journaler


*** Keywords ***
Setup
    Install Base For Component Tests
    Install Event Journaler Directly from SDDS

Custom Teardown For Multi Pub Test
    Event Journaler Teardown
    Terminate All Processes







*** Settings ***
Resource  EventJournalerResources.robot
Suite Setup  Setup
Test Teardown   Event Journaler Teardown
Suite Teardown  Uninstall Base

*** Test Cases ***
Event Journaler Can Receive Events
    Run Shell Process  chmod a+x ${EVENT_PUB_SUB_TOOL}  OnError=Failed to chmod EventPubSub binary   timeout=3s
    Run Shell Process  ${EVENT_PUB_SUB_TOOL} -s /opt/sophos-spl/var/ipc/events.ipc send  OnError=Failed to run EventPubSub binary   timeout=60s

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  5 secs
    ...  Check Event Journaler Log contains  {"threatName":"EICAR-AV-Test","threatPath":"/home/admin/eicar.com"}


Event Journaler Can Receive Events From Multiple Publishers
    [Teardown]  Custom Teardown For Multi Pub Test
    Run Shell Process  chmod a+x ${EVENT_PUB_SUB_TOOL}  OnError=Failed to chmod EventPubSub binary   timeout=3s
    Start Process 	timeout 30s ${EVENT_PUB_SUB_TOOL} -s /opt/sophos-spl/var/ipc/events.ipc send -d "msg-from-pub1" -c 100  alias=publisher1  shell=True
    Start Process 	timeout 30s ${EVENT_PUB_SUB_TOOL} -s /opt/sophos-spl/var/ipc/events.ipc send -d "msg-from-pub2" -c 100  alias=publisher2  shell=True
    Start Process 	timeout 30s ${EVENT_PUB_SUB_TOOL} -s /opt/sophos-spl/var/ipc/events.ipc send -d "msg-from-pub3" -c 100  alias=publisher3  shell=True

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Event Journaler Log contains  msg-from-pub1

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Event Journaler Log contains  msg-from-pub2

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Event Journaler Log contains  msg-from-pub3


Event Journaler Can Receive Many Events From Publisher
    [Teardown]  Custom Teardown For Multi Pub Test
    Run Shell Process  chmod a+x ${EVENT_PUB_SUB_TOOL}  OnError=Failed to chmod EventPubSub binary   timeout=3s
    Start Process 	timeout 30s ${EVENT_PUB_SUB_TOOL} -s /opt/sophos-spl/var/ipc/events.ipc send -d "msg-from-pub1" -c 100  alias=publisher1  shell=True
    Wait Until Log Contains String X Times  msg-from-pub1  100


Event Journaler Can be stopped and started
    Restart Event Journaler


*** Keywords ***
Setup
    Install Base For Component Tests
    Install Event Journaler Directly from SDDS

Custom Teardown For Multi Pub Test
    Event Journaler Teardown
    Terminate All Processes







*** Settings ***
Resource  EventJournalerResources.robot
Suite Setup  Setup
Test Teardown   Event Journaler Teardown
Suite Teardown  Uninstall Base
*** Test Cases ***
Event Journaler Can recieve events

    Run Shell Process  ${EVENT_PUB_SUB_TOOL} -s /opt/sophos-spl/plugins/eventjournaler/var/event.ipc send  OnError=Failed to run EventPubSub binary   timeout=60s
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  5 secs
    ...  Check Event Journaler Log contains  {"threatName":"EICAR-AV-Test","threatPath":"/home/admin/eicar.com"}

Event Journaler Can be stopped and started
    Restart Event Journaler

*** Keywords ***
Setup
    Install Base For Component Tests
    Install Event Journaler Directly from SDDS


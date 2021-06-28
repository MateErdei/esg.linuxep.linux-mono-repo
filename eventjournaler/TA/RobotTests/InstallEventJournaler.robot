*** Settings ***
Resource  EventJournalerResources.robot
Test Setup  Install Base For Component Tests
Test Teardown  Uninstall Base
*** Test Cases ***
Install Event Journaler
    Install Event Journaler Directly from SDDS
    Run Shell Process  ${EVENT_PUB_SUB_TOOL} -s /opt/sophos-spl/plugins/eventjournaler/var/event.ipc send
    LOG FILE  ${EVENT_JOURNALER_LOG_PATH}

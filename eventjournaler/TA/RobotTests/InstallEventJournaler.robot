*** Settings ***
Resource  EventJournalerResources.robot
Test Setup  Install Base For Component Tests
Test Teardown  Uninstall Base
*** Test Cases ***
Install Event Journaler
    Install Event Journaler Directly from SDDS

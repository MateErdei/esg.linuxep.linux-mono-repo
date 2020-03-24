*** Settings ***
Resource    ../shared/GlobalSetup.robot

Suite Setup     Product Test Setup Tasks
Suite Teardown  Global Teardown Tasks

*** Keywords ***

Product Test Setup Tasks
    Setup Component For Testing


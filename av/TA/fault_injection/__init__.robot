*** Settings ***
Resource    ../shared/ComponentSetup.robot
Resource    ../shared/GlobalSetup.robot
Resource    ../shared/AVAndBaseResources.robot

Suite Setup     Fault Injection Test Suite Setup Tasks
Suite Teardown  Global Teardown Tasks


*** Keywords ***

Fault Injection Test Suite Setup Tasks
    Install With Base SDDS


*** Settings ***
Resource    ../shared/ComponentSetup.robot
Resource    ../shared/GlobalSetup.robot

Suite Setup     Fault Injection Test Suite Setup Tasks

Test Setup      Fault Injection Test Setup

*** Keywords ***

Fault Injection Test Suite Setup Tasks
    Setup Base And Component

Fault Injection Test Setup
    Component Test Setup

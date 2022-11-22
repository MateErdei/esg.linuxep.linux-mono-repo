*** Settings ***
Resource    ../shared/ComponentSetup.robot
Resource    ../shared/GlobalSetup.robot

Suite Setup     Product Test Suite Setup Tasks

Test Setup      Product Test Setup

*** Keywords ***

Product Test Suite Setup Tasks
    Setup Base And Component

Product Test Setup
    Component Test Setup

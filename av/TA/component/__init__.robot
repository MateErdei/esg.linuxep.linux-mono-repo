*** Settings ***
Resource    ../shared/ComponentSetup.robot
Resource    ../shared/GlobalSetup.robot

Suite Setup     Product Test Setup Tasks

*** Keywords ***

Product Test Setup Tasks
    Setup Base And Component


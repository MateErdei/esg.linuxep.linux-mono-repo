*** Settings ***
Library         OperatingSystem
Resource        shared/ComponentSetup.robot
Resource        shared/GlobalSetup.robot

Test Timeout    5 minutes

Suite Setup     Global Setup Tasks

Test Setup      Component Test Setup
Test Teardown   Component Test TearDown


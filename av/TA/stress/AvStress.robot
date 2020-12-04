*** Settings ***
Documentation    Stress testing AVP Plugin
Force Tags       STRESS

Resource        ../shared/AVResources.robot
Resource        ../shared/BaseResources.robot
Resource        ../shared/AVAndBaseResources.robot

Suite Setup     Install With Base SDDS
Suite Teardown  Uninstall And Revert Setup

Test Setup      AV And Base Setup
Test Teardown   AV And Base Teardown

*** Test Cases ***

AV plugin runs scan now and completes
     Create EICAR files  1000  /tmp_test/stress
     Check AV Plugin Installed With Base
     Configure and check scan now
     ${result} =  Count AV Log Lines
     Should be true  ${result} > 1000
     [Teardown]  Remove Directory  /tmp_test/stress  True

*** Keywords ***


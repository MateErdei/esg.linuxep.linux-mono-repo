*** Settings ***
Documentation    Suite description

Library         Process
Library         OperatingSystem

Resource        EDRResources.robot
Resource        ComponentSetup.robot

Suite Setup     Install Base For Component Tests
Suite Teardown  Uninstall And Revert Setup

Test Setup      No operation
Test Teardown   Run Keywords
...             EDR And Base Teardown AND
...             Uninstall EDR

*** Test Cases ***
EDR plugins handles invalid value for events max in plugin conf
    Create File  ${SOPHOS_INSTALL}/plugins/edr/etc/plugin.conf  events_max=invalid
    Install EDR Directly from SDDS
    Check EDR Plugin Installed With Base
    EDR Plugin Log Contains  events_max value in '/opt/sophos-spl/plugin/edr/etc/plugin.conf' not a integer, defaulting to

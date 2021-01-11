*** Settings ***
Documentation    Suite description

Library         Process
Library         OperatingSystem

Resource        EDRResources.robot
Resource        ComponentSetup.robot

Suite Setup     Install Base For Component Tests
Suite Teardown  Uninstall And Revert Setup

Test Setup      Install EDR Directly from SDDS
Test Teardown   Run Keywords
...             EDR And Base Teardown  AND
...             Uninstall EDR

*** Test Cases ***
EDR plugins handles invalid value for events max in plugin conf
    Remove File  ${SOPHOS_INSTALL}/plugins/edr/etc/plugin.conf
    Create File  ${SOPHOS_INSTALL}/plugins/edr/etc/plugin.conf  events_max=invalid\n
    Restart EDR
    Check EDR Plugin Installed With Base
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  EDR Plugin Log Contains  events_max value in '/opt/sophos-spl/plugins/edr/etc/plugin.conf' not a integer, defaulting to


EDR plugins handles valid value for events max in plugin conf
    Remove File  ${SOPHOS_INSTALL}/plugins/edr/etc/plugin.conf
    Create File  ${SOPHOS_INSTALL}/plugins/edr/etc/plugin.conf  events_max=4000\n
    Restart EDR
    Check EDR Plugin Installed With Base
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  EDR Plugin Log Contains  Setting events_max to 4000 as per value in
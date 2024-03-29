*** Settings ***
Documentation    Suite description

Library         Process
Library         OperatingSystem

Resource        EDRResources.robot

Suite Setup     Install Base For Component Tests
Suite Teardown  Uninstall All

Test Setup      Install EDR Directly from SDDS
Test Teardown   EDRPluginConf Test Teardown

Force Tags    TAP_PARALLEL2

*** Test Cases ***
EDR plugins handles invalid value for events max in plugin conf
    Remove File  ${SOPHOS_INSTALL}/plugins/edr/etc/plugin.conf
    Create File  ${SOPHOS_INSTALL}/plugins/edr/etc/plugin.conf  events_max=invalid\n
    Restart EDR
    Check EDR Plugin Installed With Base
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  EDR Plugin Log Contains  events_max value in '/opt/sophos-spl/plugins/edr/etc/plugin.conf' not an integer, so using default of

EDR plugins handles valid value for events max in plugin conf
    Remove File  ${SOPHOS_INSTALL}/plugins/edr/etc/plugin.conf
    Create File  ${SOPHOS_INSTALL}/plugins/edr/etc/plugin.conf  events_max=4000\n
    Create Debug Level Logger Config File
    Restart EDR
    Check EDR Plugin Installed With Base
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  EDR Plugin Log Contains  Setting events_max to 4000 as per value in
    Wait Until Keyword Succeeds
        ...  15 secs
        ...  1 secs
        ...  EDR Plugin Log Contains  Checking for time of oldest event, up to 4000 events

EDR plugins reads and uses watchdog flags from in plugin conf
    Remove File  ${SOPHOS_INSTALL}/plugins/edr/etc/plugin.conf
    Create File  ${SOPHOS_INSTALL}/plugins/edr/etc/plugin.conf  events_max=4000\nwatchdog_memory_limit=500\nwatchdog_utilization_limit=60\nwatchdog_latency_limit=1\nwatchdog_delay=61\n
    Create Debug Level Logger Config File
    Restart EDR
    Check EDR Plugin Installed With Base
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  EDR Plugin Log Contains  Setting events_max to 4000 as per value in
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  File Log Contains    ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.flags    --watchdog_memory_limit=500
    File Log Contains    ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.flags    watchdog_utilization_limit=60
    File Log Contains    ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.flags    watchdog_latency_limit=1
    File Log Contains    ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.flags    watchdog_delay=61

*** Keywords ***

EDRPluginConf Test Teardown
    EDR And Base Teardown Without Starting EDR
    Uninstall EDR

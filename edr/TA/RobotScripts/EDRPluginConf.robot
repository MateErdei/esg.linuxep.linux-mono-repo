*** Settings ***
Documentation    Suite description

Library         Process
Library         OperatingSystem

Resource        EDRResources.robot

Suite Setup     Install Base For Component Tests
Suite Teardown  Uninstall All

Test Setup      Install EDR Directly from SDDS
Test Teardown   Run Keywords
...             EDR And Base Teardown  AND
...             Uninstall EDR

Default Tags    TAP_PARALLEL3

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

EDR plugins reads and sues watchdog flags from in plugin conf
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

EDR plugin sets scheduled_queries_next flag in plugin.conf if not already set without logging a warning
    Check EDR Plugin Installed With Base
    Create File  ${SOPHOS_INSTALL}/base/mcs/tmp/flags.json  {"scheduled_queries.next": true}
    Move File Atomically  ${SOPHOS_INSTALL}/base/mcs/tmp/flags.json  ${SOPHOS_INSTALL}/base/mcs/policy/flags.json
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  EDR Plugin Log Contains  Setting scheduled_queries_next flag settings to: 1
    EDR Plugin Log Does Not Contain  Failed to read scheduled_queries_next configuration from config file due to error
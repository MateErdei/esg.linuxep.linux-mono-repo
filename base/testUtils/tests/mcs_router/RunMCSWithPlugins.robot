*** Settings ***
Resource  ../mcs_router/McsRouterResources.robot
Resource  ../management_agent-audit_plugin/AuditPluginResources.robot
Resource  ../management_agent-event_processor/EventProcessorResources.robot

Test Setup  Run Keywords
...         Setup MCS Tests  AND
...         Start Local Cloud Server

Test Teardown   Run Keywords
...             MCSRouter Default Test Teardown  AND
...             AuditPluginResources.Default Test Teardown  AND
...             Stop Local Cloud Server

Suite Teardown   Uninstall SSPL Unless Cleanup Disabled


Default Tags  AUDIT_PLUGIN  MCS  EVENT_PLUGIN  MCS_ROUTER

*** Test Cases ***
Successful Start Up of MCS and Installed Plugins Are Detected
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter
    Check MCS Router Running

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  File Should Exist   ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log

    Check Mcsrouter Log Does Not Contain     Plugin found: EventProcessor.json, with APPIDs: SAV
    Check Mcsrouter Log Does Not Contain     Plugin found: AuditPlugin.json, with APPIDs

    Install Audit Plugin Directly
    Install EventProcessor Plugin Directly
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  5 secs
    ...  Check Mcsrouter Log Contains     Plugin found: AuditPlugin.json, with APPIDs:

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  5 secs
    ...  Check Mcsrouter Log Contains     Plugin found: EventProcessor.json, with APPIDs: SAV



Successful Start Up of MCS and Uninstalling Plugins Are Detected
    Install EventProcessor Plugin Directly
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  File Should Exist   ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  5 secs
    ...  Check Mcsrouter Log Contains     Plugin found: EventProcessor.json, with APPIDs: SAV

    Uninstall EventProcessor Plugin

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  5 secs
    ...  Check Mcsrouter Log Contains     Plugin removed: EventProcessor.json, with APPIDs: SAV
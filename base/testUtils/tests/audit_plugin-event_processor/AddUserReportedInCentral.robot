*** Settings ***

Library     ${LIBS_DIRECTORY}/UserUtils.py
Library     ${LIBS_DIRECTORY}/LogUtils.py

Resource  ../management_agent-audit_plugin/AuditPluginResources.robot
Resource  ../management_agent-event_processor/EventProcessorResources.robot

Test Setup  Run Keywords
...         Install Audit Plugin Directly  AND
...         Install EventProcessor Plugin Directly

Test Teardown   Run Keywords
...             AuditPluginResources.Default Test Teardown  AND
...             Delete User  addUserIsReportedInCentral  True  AND
...             Uninstall Audit Plugin  AND
...             Uninstall EventProcessor Plugin  AND
...             Run Keyword If Test Failed   Require Fresh Install

Default Tags  AUDIT_PLUGIN  EVENT_PLUGIN  PUB_SUB

*** Test Cases ***

Add user is reported in Event XML
    Add User  addUserIsReportedInCentral
    ${Description}=  Set Variable  User Add Credential Event Created
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check Log Contains   ${Description}    ${SOPHOS_INSTALL}/plugins/AuditPlugin/log/AuditPlugin.log   sspl-audit.log
    ${Description}=  Set Variable  "User has been created"
    ${Name}=  Set Variable  "ATTENTION: New user has been created"
    Wait Until Keyword Succeeds
    ...  3 secs
    ...  1 secs
    ...  SAV Event File Exists And Contains Correct Contents   ${Description}  ${Name}

Delete User is logged correctly
    Add User  addUserIsReportedInCentral
    Delete User  addUserIsReportedInCentral  False
    ${Description}=  Set Variable  User Delete Credential Event Created
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check Log Contains    ${Description}   ${SOPHOS_INSTALL}/plugins/AuditPlugin/log/AuditPlugin.log    sspl-audit.log
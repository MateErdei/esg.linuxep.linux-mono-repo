*** Settings ***

Resource  ../management_agent-audit_plugin/AuditPluginResources.robot
Resource  ../installer/InstallerResources.robot

Test Setup  Run Keywords
...         Install Audit Plugin Directly

Test Teardown   Run Keywords
...             AuditPluginResources.Default Test Teardown  AND
...             Uninstall Audit Plugin  AND
...             Run Keyword If Test Failed   Require Fresh Install

Default Tags  AUDIT_PLUGIN  PUB_SUB

*** Test Cases ***

Audit Plugin Processes Remote Kerberos SSH
    ## Run the successful Kerberos SSH against audit-plugin
    Disable System Auditd
    Start Subscriber

    Run AuditPlugin Against Example  Kerberos_ssh




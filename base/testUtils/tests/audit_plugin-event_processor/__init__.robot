*** Settings ***
Resource  ../installer/InstallerResources.robot
Resource  ../watchdog/LogControlResources.robot
Resource  ../management_agent-audit_plugin/AuditPluginResources.robot

Suite Setup     Run Keywords
...             Require Fresh Install  AND
...             Set Log Level For Component And Reset and Return Previous Log  AuditPlugin  DEBUG

Suite Teardown      Uninstall And Terminate All Processes
*** Keywords ***

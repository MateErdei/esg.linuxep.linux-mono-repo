*** Settings ***
Library     Process
Library     Collections
Library     OperatingSystem

Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library     ${LIBS_DIRECTORY}/CapnSubscriber.py
Library     ${LIBS_DIRECTORY}/SshSupport.py


Resource  ../sul_downloader/SulDownloaderResources.robot
Resource  ../installer/InstallerResources.robot
Resource  ../management_agent-audit_plugin/AuditPluginResources.robot
Resource  ../management_agent-event_processor/EventProcessorResources.robot

Test Setup  Run Keywords
...         Install Audit Plugin Directly  AND
...         Install EventProcessor Plugin Directly  AND
...         Empty Directory  ${SOPHOS_INSTALL}/base/mcs/event/


Test Teardown   Run Keywords
...             AuditPluginResources.Default Test Teardown  AND
...             Uninstall Audit Plugin  AND
...             Uninstall EventProcessor Plugin  AND
...             Empty Directory  ${SOPHOS_INSTALL}/base/mcs/event/  AND
...             Run Keyword If Test Failed   Require Fresh Install

Default Tags  AUDIT_PLUGIN  EVENT_PLUGIN  PUB_SUB

*** Test Cases ***
Ten Failed Logins are reported in Event XML when using ssh with password
    [Tags]  AUDIT_PLUGIN  EXCLUDE_AWS  EVENT_PLUGIN
    Start Subscriber

    Simulate Failed Ssh Using Password
    Simulate Failed Ssh Using Password
    Simulate Failed Ssh Using Password
    Simulate Failed Ssh Using Password
    Simulate Failed Ssh Using Password

    Simulate Failed Ssh Using Password
    Simulate Failed Ssh Using Password
    Simulate Failed Ssh Using Password
    Simulate Failed Ssh Using Password
    Simulate Failed Ssh Using Password

    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  SAV Event File Exists And Contains Failed Authentication Info
    ${channel} =  Get Credential Channel
    ${message} =  Check Capn Output For A Single Message   ${channel}  sessionType=networkInteractive
    ${remoteNetworkAddress}  Get From Dictionary  ${message}  remoteNetworkAddress.address
    SAV Event File Exists And Contains Failed Authentication Info With String  from ${remoteNetworkAddress}


Ten Failed Logins are reported in Event XML
    Start Subscriber

    Simulate Failed Ssh Using PublicKey Ignore Paramiko Banner Error
    Simulate Failed Ssh Using PublicKey Ignore Paramiko Banner Error
    Simulate Failed Ssh Using PublicKey Ignore Paramiko Banner Error
    Simulate Failed Ssh Using PublicKey Ignore Paramiko Banner Error
    Simulate Failed Ssh Using PublicKey Ignore Paramiko Banner Error

    Simulate Failed Ssh Using PublicKey Ignore Paramiko Banner Error
    Simulate Failed Ssh Using PublicKey Ignore Paramiko Banner Error
    Simulate Failed Ssh Using PublicKey Ignore Paramiko Banner Error
    Simulate Failed Ssh Using PublicKey Ignore Paramiko Banner Error
    Simulate Failed Ssh Using PublicKey Ignore Paramiko Banner Error

    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  SAV Event File Exists And Contains Failed Authentication Info
    ${channel} =  Get Credential Channel
    ${message} =  Check Capn Output For A Single Message   ${channel}  sessionType=networkInteractive
    ${remoteNetworkAddress}  Get From Dictionary  ${message}  remoteNetworkAddress.address
    SAV Event File Exists And Contains Failed Authentication Info With String  from ${remoteNetworkAddress}


*** Keywords ***
Simulate Failed Ssh Using PublicKey Ignore Paramiko Banner Error
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Simulate Failed Ssh Using PublicKey
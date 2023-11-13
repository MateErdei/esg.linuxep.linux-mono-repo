*** Settings ***
Suite Setup      EDR Suite Setup
Suite Teardown   EDR Suite Teardown

Test Setup       Require Uninstalled
Test Teardown    EDR Test Teardown

Library     ${COMMON_TEST_LIBS}/WarehouseUtils.py
Library     ${COMMON_TEST_LIBS}/ThinInstallerUtils.py
Library     ${COMMON_TEST_LIBS}/FullInstallerUtils.py
Library     ${COMMON_TEST_LIBS}/LogUtils.py
Library     ${COMMON_TEST_LIBS}/MCSRouter.py

Resource    ${COMMON_TEST_ROBOT}/AVResources.robot
Resource    ${COMMON_TEST_ROBOT}/EDRResources.robot
Resource    ${COMMON_TEST_ROBOT}/EventJournalerResources.robot
Resource    ${COMMON_TEST_ROBOT}/GeneralTeardownResource.robot
Resource    ${COMMON_TEST_ROBOT}/LiveResponseResources.robot
Resource    ${COMMON_TEST_ROBOT}/LogControlResources.robot
Resource    ${COMMON_TEST_ROBOT}/McsPushClientResources.robot
Resource    ${COMMON_TEST_ROBOT}/RuntimeDetectionsResources.robot
Resource    ${COMMON_TEST_ROBOT}/SulDownloaderResources.robot
Resource    ${COMMON_TEST_ROBOT}/UpgradeResources.robot
Resource    ${COMMON_TEST_ROBOT}/WatchdogResources.robot

Force Tags   FAKE_CLOUD   THIN_INSTALLER  INSTALLER  LOAD4

*** Variables ***
${EDR_STATUS_XML}                   ${SOPHOS_INSTALL}/base/mcs/status/LiveQuery_status.xml
${IPC_FILE} =                       ${SOPHOS_INSTALL}/var/ipc/plugins/edr.ipc
${CACHED_STATUS_XML} =              ${SOPHOS_INSTALL}/base/mcs/status/cache/LiveQuery.xml
${WDCTL_LOG_PATH}                   ${SOPHOS_INSTALL}/logs/base/wdctl.log
${WATCHDOG_LOG_PATH}                ${SOPHOS_INSTALL}/logs/base/watchdog.log
${Sophos_Scheduled_Query_Pack}      ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf

*** Test Cases ***
Install all plugins 999 then downgrade to all plugins develop
    [Tags]  BASE_DOWNGRADE  THIN_INSTALLER  INSTALLER  UNINSTALLER

    Setup SUS all 999
    Install EDR SDDS3  ${SUPPORT_FILES}/CentralXml/FakeCloudDefaultPolicies/FakeCloudDefault_ALC_policy.xml
    Wait Until EDR OSQuery Running  30

    Check Log Does Not Contain    wdctl <> stop edr     ${WDCTL_LOG_PATH}  WatchDog

    Wait for first update

    ${contents} =  Get File  ${DEVICESOLATION_DIR}/VERSION.ini
    Should contain   ${contents}   PRODUCT_VERSION = 99.99.99
    ${contents} =  Get File  ${EDR_DIR}/VERSION.ini
    Should contain   ${contents}   PRODUCT_VERSION = 99.99.99
    ${contents} =  Get File  ${LIVERESPONSE_DIR}/VERSION.ini
    Should contain   ${contents}   PRODUCT_VERSION = 99.99.99
    ${contents} =  Get File  ${EVENTJOURNALER_DIR}/VERSION.ini
    Should contain   ${contents}   PRODUCT_VERSION = 99.99.99
    ${contents} =  Get File  ${RESPONSE_ACTIONS_DIR}/VERSION.ini
    Should contain   ${contents}   PRODUCT_VERSION = 99.99.99
    ${contents} =  Get File  ${RUNTIMEDETECTIONS_DIR}/VERSION.ini
    Should contain   ${contents}   PRODUCT_VERSION = 99.99.99
    ${pre_downgrade_rtd_log} =  Get File  ${RUNTIMEDETECTIONS_DIR}/log/runtimedetections.log

    Create File  ${SOPHOS_INSTALL}/plugins/liveresponse/log/sessions.log

    Override LogConf File as Global Level  DEBUG
    ${sul_mark} =  mark_log_size  ${SULDOWNLOADER_LOG_PATH}

    Setup SUS all develop
    Trigger Update Now
    Wait Until Created    ${UPGRADING_MARKER_FILE}    90
    Wait until removed    ${UPGRADING_MARKER_FILE}    300


    wait_for_log_contains_from_mark  ${sul_mark}  Preparing ServerProtectionLinux-Base-component for downgrade

    wait_for_log_contains_from_mark  ${sul_mark}  Component ServerProtectionLinux-Base-component is being downgraded
    wait_for_log_contains_from_mark  ${sul_mark}  Component ServerProtectionLinux-Plugin-DeviceIsolation is being downgraded
    wait_for_log_contains_from_mark  ${sul_mark}  Component ServerProtectionLinux-Plugin-responseactions is being downgraded
    wait_for_log_contains_from_mark  ${sul_mark}  Component ServerProtectionLinux-Plugin-RuntimeDetections is being downgraded
    wait_for_log_contains_from_mark  ${sul_mark}  Component ServerProtectionLinux-Plugin-liveresponse is being downgraded
    wait_for_log_contains_from_mark  ${sul_mark}  Component ServerProtectionLinux-Plugin-EventJournaler is being downgraded
    wait_for_log_contains_from_mark  ${sul_mark}  Component ServerProtectionLinux-Plugin-EDR is being downgraded

    Wait Until Keyword Succeeds
    ...   200 secs
    ...   10 secs
    ...  Check Installed Plugins Are VUT Versions

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check Event Journaler Executable Running
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  EDR Plugin Is Running
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  RuntimeDetections Plugin Is Running
    ${post_downgrade_rtd_log} =  Get File  ${RUNTIMEDETECTIONS_DIR}/log/downgrade-backup/runtimedetections.log
    # First line of both logs should be the same, as rtd preserves it's logs on a downgrade
    Should Be Equal As Strings  ${post_downgrade_rtd_log.split("\n")[0]}  ${pre_downgrade_rtd_log.split("\n")[0]}

    Wait For Suldownloader To Finish
    Mark Known Upgrade Errors
    Mark Known Downgrade Errors

    File Should Exist  ${SOPHOS_INSTALL}/plugins/av/log/downgrade-backup/av.log
    File Should Exist  ${SOPHOS_INSTALL}/plugins/av/log/downgrade-backup/soapd.log
    File Should Exist  ${SOPHOS_INSTALL}/plugins/av/log/downgrade-backup/sophos_threat_detector.log
    File Should Exist  ${SOPHOS_INSTALL}/plugins/av/log/downgrade-backup/safestore.log


    # Event journaler logs
    File Should Exist  ${SOPHOS_INSTALL}/plugins/deviceisolation/log/downgrade-backup/deviceisolation.log

    # Liveresponse logs
    File Should Exist  ${SOPHOS_INSTALL}/plugins/liveresponse/log/downgrade-backup/liveresponse.log
    File Should Exist  ${SOPHOS_INSTALL}/plugins/liveresponse/log/downgrade-backup/sessions.log

    # Event journaler logs
    File Should Exist  ${SOPHOS_INSTALL}/plugins/eventjournaler/log/downgrade-backup/eventjournaler.log

    # Edr
    File Should Exist  ${SOPHOS_INSTALL}/plugins/edr/log/downgrade-backup/edr.log

    # Response actions logs
    File Should Exist  ${SOPHOS_INSTALL}/plugins/responseactions/log/downgrade-backup/responseactions.log

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical

Upgrade VUT to 999
    [Timeout]  10 minutes
    Setup SUS all develop
    Install EDR SDDS3  ${SUPPORT_FILES}/CentralXml/FakeCloudDefaultPolicies/FakeCloudDefault_ALC_policy.xml

    LogUtils.Check SulDownloader Log Contains     Installing product: ServerProtectionLinux-Plugin-EDR version: 1.
    LogUtils.Check SulDownloader Log Contains     Installing product: ServerProtectionLinux-Plugin-AV version: 1.

    check_suldownloader_log_should_not_contain    Installing product: ServerProtectionLinux-Base-component version: 99.99.99
    check_suldownloader_log_should_not_contain    Installing product: ServerProtectionLinux-Plugin-responseactions version: 99.99.99
    check_suldownloader_log_should_not_contain    Installing product: ServerProtectionLinux-Plugin-EDR version: 99.99.99
    check_suldownloader_log_should_not_contain    Installing product: ServerProtectionLinux-Plugin-AV version: 99.99.99
    check_suldownloader_log_should_not_contain    Installing product: ServerProtectionLinux-Plugin-DeviceIsolation version: 99.99.99
    check_suldownloader_log_should_not_contain    Installing product: ServerProtectionLinux-Plugin-EventJournaler version: 99.99.99
    check_suldownloader_log_should_not_contain    Installing product: ServerProtectionLinux-Plugin-liveresponse version: 99.99.99
    check_suldownloader_log_should_not_contain    Installing product: ServerProtectionLinux-Plugin-RuntimeDetections version: 99.99.99

    check_watchdog_log_does_not_contain    wdctl <> stop edr
    Override Local LogConf File Using Content  [edr]\nVERBOSITY = DEBUG\n[extensions]\nVERBOSITY = DEBUG\n[edr_osquery]\nVERBOSITY = DEBUG\n
    # "Update success" in suldownloader log checked inside "Install EDR SDDS3" keyword
    ${sul_mark} =  mark_log_size  ${SULDOWNLOADER_LOG_PATH}
    ${edr_mark} =  mark_log_size  ${EDR_LOG_PATH}

    Setup SUS all 999
    Trigger Update Now


    wait_for_log_contains_from_mark  ${sul_mark}  Successfully stopped product    120

    wait_for_log_contains_from_mark  ${sul_mark}  Installing product: ServerProtectionLinux-Base-component version: 99.99.99    120
    wait_for_log_contains_from_mark  ${sul_mark}  Product installed: ServerProtectionLinux-Base-component    180

    # When waiting for install messages, the order here may not be the actual order, although we are waiting 120 seconds
    # each time, this should take a lot less time overall, max time should be around 120 seconds total for all installing send messages
    # to appear.
    wait_for_log_contains_from_mark  ${sul_mark}    Installing product: ServerProtectionLinux-Plugin-liveresponse version: 99.99.99             120
    wait_for_log_contains_from_mark  ${sul_mark}    Installing product: ServerProtectionLinux-Plugin-EDR version: 99.99.99                        120
    wait_for_log_contains_from_mark  ${sul_mark}    Installing product: ServerProtectionLinux-Plugin-AV version: 99.99.99                         120
    wait_for_log_contains_from_mark  ${sul_mark}    Installing product: ServerProtectionLinux-Plugin-DeviceIsolation version: 99.99.99             120
    wait_for_log_contains_from_mark  ${sul_mark}    Installing product: ServerProtectionLinux-Plugin-EventJournaler version: 99.99.99             120
    wait_for_log_contains_from_mark  ${sul_mark}    Installing product: ServerProtectionLinux-Plugin-RuntimeDetections version: 99.99.99     120
    wait_for_log_contains_from_mark  ${sul_mark}    Installing product: ServerProtectionLinux-Plugin-responseactions version: 99.99.99        120

    wait_for_log_contains_from_mark  ${sul_mark}  Successfully started product    120

    # check plugins are running.
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  EDR Plugin Is Running

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Check AV Plugin Installed

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check Live Response Plugin Running

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check Event Journaler Executable Running

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  RuntimeDetections Plugin Is Running

    # wait for current update to complete.
    wait_for_log_contains_from_mark  ${sul_mark}    Update success    ${200}

    # Check for warning that there is a naming collision in the map of query tags
    wait_for_log_contains_from_mark   ${edr_mark}  Adding XDR results to intermediary file  timeout=${360}
    Check Edr Log Does Not Contain  already in query map

    ${base_version_contents} =  Get File  ${SOPHOS_INSTALL}/base/VERSION.ini
    Should contain   ${base_version_contents}   PRODUCT_VERSION = 99.99.99
    ${ra_version_contents} =  Get File  ${RESPONSE_ACTIONS_DIR}/VERSION.ini
    Should contain   ${ra_version_contents}   PRODUCT_VERSION = 99.99.99
    ${edr_version_contents} =  Get File  ${EDR_DIR}/VERSION.ini
    Should contain   ${edr_version_contents}   PRODUCT_VERSION = 99.99.99
    ${av_version_contents} =  Get File   ${AV_DIR}/VERSION.ini
    Should contain   ${av_version_contents}   PRODUCT_VERSION = 99.99.99
    ${live_response_version_contents} =  Get File  ${LIVERESPONSE_DIR}/VERSION.ini
    Should contain   ${live_response_version_contents}   PRODUCT_VERSION = 99.99.99
    ${event_journaler_version_contents} =  Get File  ${EVENTJOURNALER_DIR}/VERSION.ini
    Should contain   ${event_journaler_version_contents}   PRODUCT_VERSION = 99.99.99
    ${device_isolation_version_contents} =  Get File  ${DEVICEISOLATION_DIR}/VERSION.ini
    Should contain   ${device_isolation_version_contents}   PRODUCT_VERSION = 99.99.99
    ${event_journaler_version_contents} =  Get File  ${RUNTIMEDETECTIONS_DIR}/VERSION.ini
    Should contain   ${event_journaler_version_contents}   PRODUCT_VERSION = 99.99.99

    # Ensure components were restarted during update.
    Check Log Contains In Order
    ...  ${WATCHDOG_LOG_PATH}
    ...  Stopping /opt/sophos-spl/plugins/edr/bin/edr
    ...  Starting /opt/sophos-spl/plugins/edr/bin/edr


    Check Log Contains In Order
    ...  ${WATCHDOG_LOG_PATH}
    ...  Stopping /opt/sophos-spl/plugins/av/sbin/av
    ...  Starting /opt/sophos-spl/plugins/av/sbin/av

    Check Log Contains In Order
    ...  ${WATCHDOG_LOG_PATH}
    ...  Stopping /opt/sophos-spl/plugins/liveresponse/bin/liveresponse
    ...  Starting /opt/sophos-spl/plugins/liveresponse/bin/liveresponse

    Check Log Contains In Order
    ...  ${WATCHDOG_LOG_PATH}
    ...  Stopping /opt/sophos-spl/plugins/eventjournaler/bin/eventjournaler
    ...  Starting /opt/sophos-spl/plugins/eventjournaler/bin/eventjournaler

    Check Log Contains In Order
    ...  ${WATCHDOG_LOG_PATH}
    ...  Stopping /opt/sophos-spl/plugins/deviceisolation/bin/deviceisolation
    ...  Starting /opt/sophos-spl/plugins/deviceisolation/bin/deviceisolation

    Check Log Contains In Order
    ...  ${WATCHDOG_LOG_PATH}
    ...  Stopping /opt/sophos-spl/plugins/runtimedetections/bin/runtimedetections
    ...  Starting /opt/sophos-spl/plugins/runtimedetections/bin/runtimedetections

    Check Log Contains In Order
    ...  ${WATCHDOG_LOG_PATH}
    ...  Stopping /opt/sophos-spl/plugins/responseactions/bin/responseactions
    ...  Starting /opt/sophos-spl/plugins/responseactions/bin/responseactions
    #Log SSPLAV logs to the test report
    Log File      ${AV_LOG_FILE}
    Log File      ${THREAT_DETECTOR_LOG_PATH}

    Wait For Suldownloader To Finish
    Mark Known Upgrade Errors
    # Specific to this test:
    #TODO LINUXDAR-5140 remove when this defect is closed
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/av.log  ScanProcessMonitor <> failure in ConfigMonitor: pselect failed: Bad file descriptor
    #TODO LINUXDAR-6371 remove when this defect is closed
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log  managementagent <> Failure on sending message to runtimedetections. Reason: No incoming data
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log  managementagent <> Failure on sending message to mtr. Reason: No incoming data
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log  managementagent <> Failure on sending message to edr. Reason: No incoming data
    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical

Install Base And Edr Vut Then Transition To Base Edr And AV Vut
    ${sul_mark} =    mark_log_size    ${SULDOWNLOADER_LOG_PATH}
    Setup SUS all develop
    Install EDR SDDS3  ${SUPPORT_FILES}/CentralXml/ALC_policy/ALC_policy_no_av.xml

    ${statusPath}=  Set Variable  ${MCS_DIR}/status/ALC_status.xml
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Should Exist    ${statusPath}
    #Remove status file and expect that it will not be regenerated until a change in policy
    Remove File  ${statusPath}
    Should Not Exist    ${statusPath}
    # ensure EDR plugin is installed and running
    Wait For EDR to be Installed

    wait_for_log_contains_from_mark    ${sul_mark}    Update success    150
    ${sul_mark2} =    mark_log_size    ${SULDOWNLOADER_LOG_PATH}

    # Install AV
    send_policy_file  alc  ${SUPPORT_FILES}/CentralXml/FakeCloudDefaultPolicies/FakeCloudDefault_ALC_policy.xml

    Wait Until Keyword Succeeds
    ...  40 secs
    ...  5 secs
    ...  Check SulDownloader Log Contains     Installing product: ServerProtectionLinux-Plugin-AV

    wait_for_log_contains_from_mark    ${sul_mark2}    Update success    60


    Wait Until Keyword Succeeds
    ...  30
    ...  5
    ...  Check AV and Base Components Inside The ALC Status

    EDR Plugin Is Running
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check EDR Osquery Executable Running
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Should Exist    ${statusPath}

    Wait For Suldownloader To Finish
    Mark Known Upgrade Errors

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical


Install Base Edr And AV Vut Then Transition To Base Edr Vut
    Setup SUS all develop
    Install EDR SDDS3  ${SUPPORT_FILES}/CentralXml/FakeCloudDefaultPolicies/FakeCloudDefault_ALC_policy.xml

    # ensure initial plugins are installed and running
    Check AV Plugin Installed Directly
    Wait For EDR to be Installed

    # Transition to EDR Only
    ${sul_mark} =    mark_log_size    ${SULDOWNLOADER_LOG_PATH}
    send_policy_file  alc  ${SUPPORT_FILES}/CentralXml/ALC_policy/ALC_policy_no_av.xml

    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  Check SulDownloader Log Contains     Uninstalling plugin ServerProtectionLinux-Plugin-AV since it was removed from warehouse

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  5 secs
    ...  Check AV Plugin Executable Not Running
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...   File Should not Exist    ${AVPLUGIN_PATH}/bin/avscanner
    wait_for_log_contains_from_mark    ${sul_mark}    Update success    60

    # ensure EDR still running after update
    EDR Plugin Is Running
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check EDR Osquery Executable Running

    Wait For Suldownloader To Finish
    Mark Known Upgrade Errors

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical


*** Keywords ***

Check MCS Envelope Log For Event Success Within Nth Set Of Events Sent
    [Arguments]  ${Event_Number}
    ${string} =  Check Log And Return Nth Occurrence Between Strings  <ns:events   </ns:events>   ${SOPHOS_INSTALL}/logs/base/sophosspl/mcs_envelope.log   ${Event_Number}
    Should Contain   ${string}   &lt;number&gt;0&lt;/number&gt

Upgrade Installs EDR Twice
    Check Log Contains String N Times   ${SULDOWNLOADER_LOG_PATH}   SULDownloader Log   Installing product: ServerProtectionLinux-Plugin-EDR   2

Wait for first update
    Wait Until Keyword Succeeds
        ...   90 secs
        ...   10 secs
        ...   Check MCS Envelope Contains Event Success On N Event Sent  1

Check EDR Downgraded From 999
    ${edr_version_contents} =  Get File  ${EDR_DIR}/VERSION.ini
    Should Not Contain   ${edr_version_contents}   PRODUCT_VERSION = 99.99.99
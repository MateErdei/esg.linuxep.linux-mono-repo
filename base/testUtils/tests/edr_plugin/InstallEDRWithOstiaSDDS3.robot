*** Settings ***
Suite Setup      EDR Suite Setup
Suite Teardown   EDR Suite Teardown

Test Setup       EDR Test Setup
Test Teardown    EDR Test Teardown

Library     ${LIBS_DIRECTORY}/WarehouseUtils.py
Library     ${LIBS_DIRECTORY}/ThinInstallerUtils.py
Library     ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/MCSRouter.py

Resource    ../upgrade_product/UpgradeResources.robot
Resource    ../mdr_plugin/MDRResources.robot
Resource    ../av_plugin/AVResources.robot
Resource    ../event_journaler/EventJournalerResources.robot
Resource    ../GeneralTeardownResource.robot
Resource    ../watchdog/LogControlResources.robot
Resource    ../watchdog/WatchdogResources.robot
Resource    EDRResources.robot
Resource    ../mcs_router/McsPushClientResources.robot
Resource    ../liveresponse_plugin/LiveResponseResources.robot
Resource    ../runtimedetections_plugin/RuntimeDetectionsResources.robot
Resource    ../sul_downloader/SulDownloaderResources.robot

Default Tags   EDR_PLUGIN   OSTIA  FAKE_CLOUD   THIN_INSTALLER  INSTALLER
Force Tags  LOAD4


*** Variables ***
${querypackPolicy}              ${GeneratedWarehousePolicies}/old_query_pack.xml
${BaseAndEdrVUTPolicy}              ${GeneratedWarehousePolicies}/base_and_edr_VUT.xml
${BaseAndEdrAndMtrVUTPolicy}        ${GeneratedWarehousePolicies}/base_edr_and_mtr.xml
${BaseEdrAndMtrAndAVVUTPolicy}      ${GeneratedWarehousePolicies}/base_edr_and_mtr_and_av_VUT.xml
${BaseMtrAndEdr999Policy}              ${GeneratedWarehousePolicies}/base_mtr_vut_and_edr_999.xml
${Base999Policy}              ${GeneratedWarehousePolicies}/mtr_edr_vut_and_base_999.xml
${BaseAndMTREdr999Policy}              ${GeneratedWarehousePolicies}/base_vut_and_mtr_edr_999.xml
${BaseAndMTREdrAV999Policy}              ${GeneratedWarehousePolicies}/base_vut_and_mtr_edr_av_999.xml
${EDR_STATUS_XML}                   ${SOPHOS_INSTALL}/base/mcs/status/LiveQuery_status.xml
${IPC_FILE} =                       ${SOPHOS_INSTALL}/var/ipc/plugins/edr.ipc
${CACHED_STATUS_XML} =              ${SOPHOS_INSTALL}/base/mcs/status/cache/LiveQuery.xml
${SULDownloaderLogDowngrade}        ${SOPHOS_INSTALL}/logs/base/downgrade-backup/suldownloader.log
${WDCTL_LOG_PATH}                   ${SOPHOS_INSTALL}/logs/base/wdctl.log
${Sophos_Scheduled_Query_Pack}      ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf

*** Test Cases ***
Install all plugins 999 then downgrade to all plugins develop
    [Tags]  BASE_DOWNGRADE  OSTIA  THIN_INSTALLER  INSTALLER  UNINSTALLER  EXCLUDE_SLES12  EXCLUDE_SLES15

    Setup SUS all 999
    Install EDR SDDS3  ${BaseAndMTREdr999Policy}
    Wait Until EDR OSQuery Running  30

    Check Log Does Not Contain    wdctl <> stop edr     ${WDCTL_LOG_PATH}  WatchDog

    Wait for first update

    ${contents} =  Get File  ${EDR_DIR}/VERSION.ini
    Should contain   ${contents}   PRODUCT_VERSION = 9.99.9
    ${contents} =  Get File  ${MTR_DIR}/VERSION.ini
    Should contain   ${contents}   PRODUCT_VERSION = 9.99.9
    ${contents} =  Get File  ${LIVERESPONSE_DIR}/VERSION.ini
    Should contain   ${contents}   PRODUCT_VERSION = 99.99.99
    ${contents} =  Get File  ${EVENTJOURNALER_DIR}/VERSION.ini
    Should contain   ${contents}   PRODUCT_VERSION = 9.99.9
    ${contents} =  Get File  ${RESPONSE_ACTIONS_DIR}/VERSION.ini
    Should contain   ${contents}   PRODUCT_VERSION = 99.9.9.999
    ${contents} =  Get File  ${RUNTIMEDETECTIONS_DIR}/VERSION.ini
    Should contain   ${contents}   PRODUCT_VERSION = 999.999.999
    ${pre_downgrade_rtd_log} =  Get File  ${RUNTIMEDETECTIONS_DIR}/log/runtimedetections.log

    Override LogConf File as Global Level  DEBUG
    Setup SUS all develop
    Send ALC Policy And Prepare For Upgrade  ${BaseAndEdrAndMtrVUTPolicy}
    Wait Until Keyword Succeeds
    ...   90 secs
    ...   5 secs
    ...   File Should exist   ${UPGRADING_MARKER_FILE}
    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   File Should not exist   ${UPGRADING_MARKER_FILE}
    Wait Until Keyword Succeeds
    ...   200 secs
    ...   10 secs
    ...   Directory Should Exist   ${SOPHOS_INSTALL}/logs/base/downgrade-backup

    Check Log Contains  Preparing ServerProtectionLinux-Base-component for downgrade  ${SULDownloaderLogDowngrade}  backedup suldownloader log

    Check Log Contains  Component ServerProtectionLinux-Base-component is being downgraded   ${SULDownloaderLogDowngrade}  backedup suldownloader log
    Check Log Contains  Component ServerProtectionLinux-Plugin-responseactions is being downgraded   ${SULDownloaderLogDowngrade}  backedup suldownloader log
    Check Log Contains  Component ServerProtectionLinux-Plugin-RuntimeDetections is being downgraded   ${SULDownloaderLogDowngrade}  backedup suldownloader log
    Check Log Contains  Component ServerProtectionLinux-Plugin-liveresponse is being downgraded   ${SULDownloaderLogDowngrade}  backedup suldownloader log
    Check Log Contains  Component ServerProtectionLinux-Plugin-EventJournaler is being downgraded   ${SULDownloaderLogDowngrade}  backedup suldownloader log
    Check Log Contains  Component ServerProtectionLinux-Plugin-EDR is being downgraded   ${SULDownloaderLogDowngrade}  backedup suldownloader log
    Check Log Contains  Component ServerProtectionLinux-Plugin-MDR is being downgraded   ${SULDownloaderLogDowngrade}  backedup suldownloader log

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
    ${post_downgrade_rtd_log} =  Get File  ${RUNTIMEDETECTIONS_DIR}/log/runtimedetections.log
    # First line of both logs should be the same, as rtd preserves it's logs on a downgrade
    Should Be Equal As Strings  ${post_downgrade_rtd_log.split("\n")[0]}  ${pre_downgrade_rtd_log.split("\n")[0]}

    Wait For Suldownloader To Finish
    Mark Known Upgrade Errors

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical

Update Run that Does Not Change The Product Does not ReInstall The Product
    Setup SUS all develop
    Install EDR SDDS3  ${BaseAndEdrAndMtrVUTPolicy}

    Check SulDownloader Log Contains     Installing product: ServerProtectionLinux-Plugin-MDR version: 1.
    Wait for first update
    Prepare Installation For Upgrade Using Policy   ${BaseAndEdrAndMtrVUTPolicy}

    Override LogConf File as Global Level  DEBUG

    Trigger Update Now

    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  Check Log Contains String N Times   ${SULDOWNLOADER_LOG_PATH}   SULDownloader Log   Generating the report file in   2

    Run Keyword And Expect Error   *1 times not the requested 2 times*   Upgrade Installs EDR Twice

    Check MDR Plugin Installed
    Check Event Journaler Installed

    Wait For Suldownloader To Finish
    Mark Known Upgrade Errors

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical

Upgrade VUT to 999
    [Timeout]  10 minutes
    Setup SUS all develop
    Install EDR SDDS3  ${BaseEdrAndMtrAndAVVUTPolicy}

    Check SulDownloader Log Contains     Installing product: ServerProtectionLinux-Plugin-MDR version: 1.
    Check SulDownloader Log Contains     Installing product: ServerProtectionLinux-Plugin-EDR version: 1.
    Check SulDownloader Log Contains     Installing product: ServerProtectionLinux-Plugin-AV version: 1.

    check_suldownloader_log_should_not_contain    Installing product: ServerProtectionLinux-Base-component version: 99.9.9
    check_suldownloader_log_should_not_contain    Installing product: ServerProtectionLinux-Plugin-responseactions version: 99.9.9
    check_suldownloader_log_should_not_contain    Installing product: ServerProtectionLinux-Plugin-MDR version: 9.99.9
    check_suldownloader_log_should_not_contain    Installing product: ServerProtectionLinux-Plugin-EDR version: 9.99.9
    check_suldownloader_log_should_not_contain    Installing product: ServerProtectionLinux-Plugin-AV version: 9.99.9
    check_suldownloader_log_should_not_contain    Installing product: ServerProtectionLinux-Plugin-EventJournaler version: 9.99.9
    check_suldownloader_log_should_not_contain    Installing product: ServerProtectionLinux-Plugin-liveresponse version: 99.99.99
    check_suldownloader_log_should_not_contain    Installing product: ServerProtectionLinux-Plugin-RuntimeDetections version: 999.999.999

    Check Log Does Not Contain    wdctl <> stop edr     ${WDCTL_LOG_PATH}  WatchDog
    Override Local LogConf File Using Content  [edr]\nVERBOSITY = DEBUG\n[extensions]\nVERBOSITY = DEBUG\n[edr_osquery]\nVERBOSITY = DEBUG\n
    Wait for first update

    Check SulDownloader Log Contains String N Times   Update success  2
    ${sul_mark} =  mark_log_size  ${SULDOWNLOADER_LOG_PATH}

    Setup SUS all 999
    Send ALC Policy And Prepare For Upgrade  ${BaseAndMTREdrAV999Policy}
    #truncate log so that check mdr plugin installed works correctly later in the test
    ${result} =  Run Process   truncate   -s   0   ${MTR_DIR}/log/mtr.log

    wait_for_log_contains_from_mark  ${sul_mark}  Successfully stopped product    120

    wait_for_log_contains_from_mark  ${sul_mark}  Installing product: ServerProtectionLinux-Base-component version: 99.9.9    120
    wait_for_log_contains_from_mark  ${sul_mark}  Product installed: ServerProtectionLinux-Base-component    180

    # When waiting for install messages, the order here may not be the actual order, although we are waiting 120 seconds
    # each time, this should take a lot less time overall, max time should be around 120 seconds total for all installing send messages
    # to appear.
    wait_for_log_contains_from_mark  ${sul_mark}    Installing product: ServerProtectionLinux-Plugin-liveresponse version: 99.99.99             120
    wait_for_log_contains_from_mark  ${sul_mark}    Installing product: ServerProtectionLinux-Plugin-EDR version: 9.99.9                        120
    wait_for_log_contains_from_mark  ${sul_mark}    Installing product: ServerProtectionLinux-Plugin-MDR version: 9.99.9                        120
    wait_for_log_contains_from_mark  ${sul_mark}    Installing product: ServerProtectionLinux-Plugin-AV version: 9.99.9                         120
    wait_for_log_contains_from_mark  ${sul_mark}    Installing product: ServerProtectionLinux-Plugin-EventJournaler version: 9.99.9             120
    wait_for_log_contains_from_mark  ${sul_mark}    Installing product: ServerProtectionLinux-Plugin-RuntimeDetections version: 999.999.999     120
    wait_for_log_contains_from_mark  ${sul_mark}    Installing product: ServerProtectionLinux-Plugin-responseactions version: 99.9.9.999        120

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

    Check MDR Plugin Installed

    # wait for current update to complete.
    wait_for_log_contains_from_mark  ${sul_mark}    Update success    200

    # Check for warning that there is a naming collision in the map of query tags
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  2 secs
    ...  Check EDR Log Contains  Adding XDR results to intermediary file
    Check Edr Log Does Not Contain  already in query map

    ${base_version_contents} =  Get File  ${SOPHOS_INSTALL}/base/VERSION.ini
    Should contain   ${base_version_contents}   PRODUCT_VERSION = 99.9.9
    ${ra_version_contents} =  Get File  ${RESPONSE_ACTIONS_DIR}/VERSION.ini
    Should contain   ${ra_version_contents}   PRODUCT_VERSION = 99.9.9
    ${edr_version_contents} =  Get File  ${EDR_DIR}/VERSION.ini
    Should contain   ${edr_version_contents}   PRODUCT_VERSION = 9.99.9
    ${mtr_version_contents} =  Get File  ${MTR_DIR}/VERSION.ini
    Should contain   ${mtr_version_contents}   PRODUCT_VERSION = 9.99.9
    ${av_version_contents} =  Get File   ${AV_DIR}/VERSION.ini
    Should contain   ${av_version_contents}   PRODUCT_VERSION = 9.99.9
    ${live_response_version_contents} =  Get File  ${LIVERESPONSE_DIR}/VERSION.ini
    Should contain   ${live_response_version_contents}   PRODUCT_VERSION = 99.99.99
    ${event_journaler_version_contents} =  Get File  ${EVENTJOURNALER_DIR}/VERSION.ini
    Should contain   ${event_journaler_version_contents}   PRODUCT_VERSION = 9.99.9
    ${event_journaler_version_contents} =  Get File  ${RUNTIMEDETECTIONS_DIR}/VERSION.ini
    Should contain   ${event_journaler_version_contents}   PRODUCT_VERSION = 999.999.999

    # Ensure components were restarted during update.
    Check Log Contains In Order
    ...  ${WDCTL_LOG_PATH}
    ...  wdctl <> stop edr
    ...  wdctl <> start edr

    Check Log Contains In Order
    ...  ${WDCTL_LOG_PATH}
    ...  wdctl <> stop mtr
    ...  wdctl <> start mtr

    Check Log Contains In Order
    ...  ${WDCTL_LOG_PATH}
    ...  wdctl <> stop av
    ...  wdctl <> start av

    Check Log Contains In Order
    ...  ${WDCTL_LOG_PATH}
    ...  wdctl <> stop liveresponse
    ...  wdctl <> start liveresponse

    Check Log Contains In Order
    ...  ${WDCTL_LOG_PATH}
    ...  wdctl <> stop eventjournaler
    ...  wdctl <> start eventjournaler

    Check Log Contains In Order
    ...  ${WDCTL_LOG_PATH}
    ...  wdctl <> stop eventjournaler
    ...  wdctl <> start eventjournaler

    Check Log Contains In Order
    ...  ${WDCTL_LOG_PATH}
    ...  wdctl <> stop responseactions
    ...  wdctl <> start responseactions
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

Install develop of base and edr and mtr and upgrade to base 999
    Setup SUS all develop
    Install EDR SDDS3  ${BaseAndEdrAndMtrVUTPolicy}

    Check log Does not Contain   Installing product: ServerProtectionLinux-Base-component version: 99.9.9   ${SULDOWNLOADER_LOG_PATH}  Sul-Downloader

    Setup SUS only base 999
    Send ALC Policy And Prepare For Upgrade  ${Base999Policy}
    #truncate log so that check mdr plugin installed works correctly later in the test
    ${result} =  Run Process   truncate   -s   0   ${MTR_DIR}/log/mtr.log

    Wait Until Keyword Succeeds
    ...   90 secs
    ...   1 secs
    ...   File Should exist   ${UPGRADING_MARKER_FILE}
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check SulDownloader Log Contains     Installing product: ServerProtectionLinux-Base-component version: 99.9.9

    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   File Should not exist   ${UPGRADING_MARKER_FILE}
    Wait Until Keyword Succeeds
    ...   200 secs
    ...   2 secs
    ...   Check Log Contains String At Least N times   ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Update success  2
    # check plugins are running.
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  EDR Plugin Is Running

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

    Check MDR Plugin Installed

    ${base_version_contents} =  Get File  ${SOPHOS_INSTALL}/base/VERSION.ini
    Should contain   ${base_version_contents}   PRODUCT_VERSION = 99.9.9

    Wait For Suldownloader To Finish
    Mark Known Upgrade Errors

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical


Install Base And Edr Vut Then Transition To Base Edr And Mtr Vut
    Setup SUS all develop
    Install EDR SDDS3  ${BaseAndEdrVUTPolicy}

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

    # ensure MTR is not installed.
    Wait Until MDR Uninstalled

    # Install MTR
    Send ALC Policy And Prepare For Upgrade  ${BaseAndEdrAndMtrVUTPolicy}

    Wait Until Keyword Succeeds
    ...  40 secs
    ...  5 secs
    ...  Check SulDownloader Log Contains     Installing product: ServerProtectionLinux-Plugin-MDR

    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...   Check SulDownloader Log Contains String N Times   Update success  3

    Wait Until MDR Installed

    # ensure Plugins are running after update
    Check MDR Plugin Running
    Check MDR and Base Components Inside The ALC Status

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

Install Base Edr And Mtr Vut Then Transition To Base Edr Vut
    Setup SUS all develop
    Install EDR SDDS3  ${BaseAndEdrAndMtrVUTPolicy}

    # ensure initial plugins are installed and running
    Wait Until MDR Installed
    Wait For EDR to be Installed

    # Transition to EDR Only
    Send ALC Policy And Prepare For Upgrade  ${BaseAndEdrVUTPolicy}

    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  Check SulDownloader Log Contains     Uninstalling plugin ServerProtectionLinux-Plugin-MDR since it was removed from warehouse

    Wait Until MDR Uninstalled

    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  Check SulDownloader Log Contains String N Times   Update success  3

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
    Should Not Contain   ${edr_version_contents}   PRODUCT_VERSION = 9.99.9

Check Installed Plugins Are VUT Versions
    ${contents} =  Get File  ${EDR_DIR}/VERSION.ini
    ${edr_vut_version} =  get_version_for_rigidname_in_vut_warehouse   ServerProtectionLinux-Plugin-EDR
    Should Contain   ${contents}   PRODUCT_VERSION = ${edr_vut_version}
    ${contents} =  Get File  ${MTR_DIR}/VERSION.ini
    ${mtr_vut_version} =  get_version_for_rigidname_in_vut_warehouse   ServerProtectionLinux-Plugin-MDR
    Should Contain   ${contents}   PRODUCT_VERSION = ${mtr_vut_version}
    ${contents} =  Get File  ${LIVERESPONSE_DIR}/VERSION.ini
    ${liveresponse_vut_version} =  get_version_for_rigidname_in_vut_warehouse   ServerProtectionLinux-Plugin-liveresponse
    Should Contain   ${contents}   PRODUCT_VERSION = ${liveresponse_vut_version}
    ${contents} =  Get File  ${EVENTJOURNALER_DIR}/VERSION.ini
    ${eventjournaler_vut_version} =  get_version_for_rigidname_in_vut_warehouse   ServerProtectionLinux-Plugin-EventJournaler
    Should Contain   ${contents}   PRODUCT_VERSION = ${eventjournaler_vut_version}
    ${contents} =  Get File  ${RUNTIMEDETECTIONS_DIR}/VERSION.ini
    ${runtimedetections_vut_version} =  get_version_for_rigidname_in_vut_warehouse   ServerProtectionLinux-Plugin-RuntimeDetections
    Should Contain   ${contents}   PRODUCT_VERSION = ${runtimedetections_vut_version}

Wait For Suldownloader To Finish
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check Suldownloader Is Not Running

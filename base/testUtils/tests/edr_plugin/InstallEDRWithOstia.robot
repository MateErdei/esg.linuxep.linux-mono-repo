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
Resource    EDRResources.robot
Resource    ../mcs_router/McsPushClientResources.robot
Resource    ../liveresponse_plugin/LiveResponseResources.robot
Resource    ../runtimedetections_plugin/RuntimeDetectionsResources.robot
Resource    ../sul_downloader/SulDownloaderResources.robot

Default Tags   EDR_PLUGIN   OSTIA  FAKE_CLOUD   THIN_INSTALLER  INSTALLER
Force Tags  LOAD1


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
${SULDOWNLOADER_LOG_PATH}           ${SOPHOS_INSTALL}/logs/base/suldownloader.log
${SULDownloaderLogDowngrade}        ${SOPHOS_INSTALL}/logs/base/downgrade-backup/suldownloader.log
${WDCTL_LOG_PATH}                   ${SOPHOS_INSTALL}/logs/base/wdctl.log
${Sophos_Scheduled_Query_Pack}      ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf

*** Test Cases ***
Install all plugins 999 then downgrade to all plugins develop
    [Tags]  BASE_DOWNGRADE  OSTIA  THIN_INSTALLER  INSTALLER  UNINSTALLER
    Install EDR  ${BaseAndMTREdr999Policy}
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
    ${contents} =  Get File  ${RUNTIMEDETECTIONS_DIR}/VERSION.ini
    Should contain   ${contents}   PRODUCT_VERSION = 999.999.999
    ${pre_downgrade_rtd_log} =  Get File  ${RUNTIMEDETECTIONS_DIR}/log/runtimedetections.log

    Override LogConf File as Global Level  DEBUG

    Send ALC Policy And Prepare For Upgrade  ${BaseAndEdrAndMtrVUTPolicy}
    Wait Until Keyword Succeeds
    ...   90 secs
    ...   5 secs
    ...   File Should exist   ${SOPHOS_INSTALL}/base/update/var/updatescheduler/upgrade_marker_file
    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   File Should not exist   ${SOPHOS_INSTALL}/base/update/var/updatescheduler/upgrade_marker_file
    Wait Until Keyword Succeeds
    ...   200 secs
    ...   10 secs
    ...   Directory Should Exist   ${SOPHOS_INSTALL}/logs/base/downgrade-backup

    Check Log Contains  Preparing ServerProtectionLinux-Base-component for downgrade  ${SULDownloaderLogDowngrade}  backedup suldownloader log

    Check Log Contains  Component ServerProtectionLinux-Base-component is being downgraded   ${SULDownloaderLogDowngrade}  backedup suldownloader log
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

Install edr 999 and downgrade to current edr
    [Tags]  PLUGIN_DOWNGRADE  OSTIA  THIN_INSTALLER  INSTALLER  UNINSTALLER
    Install EDR  ${BaseMtrAndEdr999Policy}

    Check SulDownloader Log Contains     Installing product: ServerProtectionLinux-Plugin-EDR version: 9.99.9

    ${edr_version_contents} =  Get File  ${EDR_DIR}/VERSION.ini
    Should contain   ${edr_version_contents}   PRODUCT_VERSION = 9.99.9
    Override LogConf File as Global Level  DEBUG

    Wait for first update
    # This policy change will trigger another update automatically
    Send ALC Policy And Prepare For Upgrade  ${BaseAndEdrAndMtrVUTPolicy}

    Wait Until Keyword Succeeds
    ...  120 secs
    ...  5 secs
    ...  Check SulDownloader Log Contains     Installing product: ServerProtectionLinux-Plugin-EDR version: 1.

    Check SulDownloader Log Contains     Prepared ServerProtectionLinux-Plugin-EDR for downgrade

    # SulDownloader log may be removed during downgrade, therefore check edr version file is no longer
    # reporting the 999 version and make sure edr is left running.

    Wait Until Keyword Succeeds
    ...   200 secs
    ...   2 secs
    ...   Check EDR Downgraded From 999

    Wait Until Keyword Succeeds
    ...   15 secs
    ...   2 secs
    ...   Check EDR Executable Running

    Wait For Suldownloader To Finish
    Mark Known Upgrade Errors

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical

Update Run that Does Not Change The Product Does not ReInstall The Product
    Install EDR  ${BaseAndEdrAndMtrVUTPolicy}

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

Install master of base and edr and mtr and upgrade to edr 999
    Install EDR  ${BaseAndEdrAndMtrVUTPolicy}

    Override Local LogConf File Using Content  [edr]\nVERBOSITY = DEBUG\n[extensions]\nVERBOSITY = DEBUG\n[edr_osquery]\nVERBOSITY = DEBUG\n

    Check SulDownloader Log Contains     Installing product: ServerProtectionLinux-Plugin-EDR version: 1.
    Check Log Does Not Contain    Installing product: ServerProtectionLinux-Plugin-EDR version: 9.99.9     ${SULDOWNLOADER_LOG_PATH}  Sul-Downloader

    Check Log Does Not Contain    wdctl <> stop edr     ${WDCTL_LOG_PATH}  WatchDog

    Wait for first update

    Mark Edr Log
    Send ALC Policy And Prepare For Upgrade  ${BaseMtrAndEdr999Policy}

    Wait Until Keyword Succeeds
    ...  150 secs
    ...  5 secs
    ...  Check SulDownloader Log Contains     Installing product: ServerProtectionLinux-Plugin-EDR version: 9.99.9

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  EDR Plugin Is Running

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check Marked EDR Log Contains  Reading /opt/sophos-spl/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.mtr.conf for query tags
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check Marked EDR Log Contains  Reading /opt/sophos-spl/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf for query tags

    Wait Until Keyword Succeeds
    ...   200 secs
    ...   2 secs
    ...   Check Log Contains String At Least N Times   ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Update success  2

    ${edr_version_contents} =  Get File  ${EDR_DIR}/VERSION.ini
    Should contain   ${edr_version_contents}   PRODUCT_VERSION = 9.99.9

    ${mtr_version_contents} =  Get File  ${MTR_DIR}/VERSION.ini
    Should not contain   ${mtr_version_contents}   PRODUCT_VERSION = 9.99.9
    ${event_journaler_version_contents} =  Get File  ${EVENTJOURNALER_DIR}/VERSION.ini
    Should not contain   ${event_journaler_version_contents}   PRODUCT_VERSION = 9.99.9
    # Ensure EDR was restarted during upgrade.
    Check Log Contains In Order
    ...  ${WDCTL_LOG_PATH}
    ...  wdctl <> stop edr
    ...  wdctl <> start edr

    Wait For Suldownloader To Finish
    Mark Known Upgrade Errors

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical

Install master of base and edr and mtr and upgrade to new query pack
    Install EDR  ${BaseAndEdrAndMtrVUTPolicy}

    Check SulDownloader Log Contains     Installing product: ServerProtectionLinux-Plugin-EDR version: 1.
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  5 secs
    ...  file should exist  ${EDR_DIR}/VERSION.ini
    ${edr_version_contents} =  Get File  ${EDR_DIR}/VERSION.ini

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  5 secs
    ...  file should exist  ${Sophos_Scheduled_Query_Pack}
    ${query_pack_vut} =  Get File  ${Sophos_Scheduled_Query_Pack}
    ${osquery_pid_before_query_pack_reload} =  Get Edr OsQuery PID
    Wait Until Keyword Succeeds
    ...   60 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  1
    Send ALC Policy And Prepare For Upgrade  ${querypackPolicy}



    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  Check Log Contains String N Times   ${SULDOWNLOADER_LOG_PATH}   SULDownloader Log   Generating the report file in   2


    # Ensure EDR was restarted during upgrade.
    Check Log Contains In Order
    ...  ${WDCTL_LOG_PATH}
    ...  wdctl <> stop edr
    ...  wdctl <> start edr

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  5 secs
    ...  file should exist  ${Sophos_Scheduled_Query_Pack}
    ${query_pack_99} =  Get File  ${Sophos_Scheduled_Query_Pack}
    ${osquery_pid_after_query_pack_reload} =  Get Edr OsQuery PID
    ${edr_version_contents1} =  Get File  ${EDR_DIR}/VERSION.ini
    Should Not Be Equal As Strings  ${query_pack_99}  ${query_pack_vut}
    Should Be Equal As Strings  ${edr_version_contents1}  ${edr_version_contents}
    Should Not Be Equal As Integers  ${osquery_pid_after_query_pack_reload}  ${osquery_pid_before_query_pack_reload}

    Wait For Suldownloader To Finish
    Mark Known Upgrade Errors

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical


Install master of base and edr and mtr and av and upgrade to edr 999 and mtr 999 and av 999
    [Timeout]  10 minutes
    Install EDR  ${BaseEdrAndMtrAndAVVUTPolicy}

    Check SulDownloader Log Contains     Installing product: ServerProtectionLinux-Plugin-MDR version: 1.
    Check SulDownloader Log Contains     Installing product: ServerProtectionLinux-Plugin-EDR version: 1.
    Check SulDownloader Log Contains     Installing product: ServerProtectionLinux-Plugin-AV version: 1.

    Check log Does not Contain   Installing product: ServerProtectionLinux-Base-component version: 99.9.9   ${SULDOWNLOADER_LOG_PATH}  Sul-Downloader
    Check Log Does Not Contain    Installing product: ServerProtectionLinux-Plugin-MDR version: 9.99.9     ${SULDOWNLOADER_LOG_PATH}  Sul-Downloader
    Check Log Does Not Contain    Installing product: ServerProtectionLinux-Plugin-EDR version: 9.99.9     ${SULDOWNLOADER_LOG_PATH}  Sul-Downloader
    Check Log Does Not Contain    Installing product: ServerProtectionLinux-Plugin-AV version: 9.99.9     ${SULDOWNLOADER_LOG_PATH}  Sul-Downloader
    Check Log Does Not Contain    Installing product: ServerProtectionLinux-Plugin-EventJournaler version: 9.99.9     ${SULDOWNLOADER_LOG_PATH}  Sul-Downloader
    Check log Does not Contain   Installing product: ServerProtectionLinux-Plugin-liveresponse version: 99.99.99   ${SULDOWNLOADER_LOG_PATH}  Sul-Downloader
    Check log Does not Contain   Installing product: ServerProtectionLinux-Plugin-RuntimeDetections version: 999.999.999   ${SULDOWNLOADER_LOG_PATH}  Sul-Downloader

    Check Log Does Not Contain    wdctl <> stop edr     ${WDCTL_LOG_PATH}  WatchDog
    Override Local LogConf File Using Content  [edr]\nVERBOSITY = DEBUG\n[extensions]\nVERBOSITY = DEBUG\n[edr_osquery]\nVERBOSITY = DEBUG\n
    Wait for first update

    Check Log Contains String N times   ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Update success  1

    Send ALC Policy And Prepare For Upgrade  ${BaseAndMTREdrAV999Policy}
    #truncate log so that check mdr plugin installed works correctly later in the test
    ${result} =  Run Process   truncate   -s   0   ${MTR_DIR}/log/mtr.log
    Mark Sul Log

    Wait Until Keyword Succeeds
    ...  120 secs
    ...  5 secs
    ...  Check SulDownloader Log Contains     Installing product: ServerProtectionLinux-Base-component version: 99.9.9

    Wait Until Keyword Succeeds
    ...  180 secs
    ...  5 secs
    ...  Check Marked Sul Log Contains     Product installed: ServerProtectionLinux-Base-component

    # When waiting for install messages, the order here may not be the actual order, although we are waiting 120 seconds
    # each time, this should take a lot less time overall, max time should be around 120 seconds total for all installing send messages
    # to appear.
    Wait Until Keyword Succeeds
    ...  120 secs
    ...  2 secs
    ...  Check SulDownloader Log Contains     Installing product: ServerProtectionLinux-Plugin-liveresponse version: 99.99.99
    Wait Until Keyword Succeeds
    ...  120 secs
    ...  2 secs
    ...  Check SulDownloader Log Contains     Installing product: ServerProtectionLinux-Plugin-EDR version: 9.99.9
    Wait Until Keyword Succeeds
    ...  120 secs
    ...  5 secs
    ...  Check SulDownloader Log Contains     Installing product: ServerProtectionLinux-Plugin-MDR version: 9.99.9
    Wait Until Keyword Succeeds
    ...  120 secs
    ...  2 secs
    ...  Check SulDownloader Log Contains     Installing product: ServerProtectionLinux-Plugin-AV version: 9.99.9
    Wait Until Keyword Succeeds
    ...  120 secs
    ...  2 secs
    ...  Check SulDownloader Log Contains     Installing product: ServerProtectionLinux-Plugin-EventJournaler version: 9.99.9
    Wait Until Keyword Succeeds
    ...  120 secs
    ...  2 secs
    ...  Check SulDownloader Log Contains     Installing product: ServerProtectionLinux-Plugin-RuntimeDetections version: 999.999.999

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
    Wait Until Keyword Succeeds
    ...   200 secs
    ...   2 secs
    ...   Check Log Contains String At Least N times   ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Update success  2

    # Check for warning that there is a naming collision in the map of query tags
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  2 secs
    ...  Check EDR Log Contains  Adding XDR results to intermediary file
    Check Edr Log Does Not Contain  already in query map

    ${base_version_contents} =  Get File  ${SOPHOS_INSTALL}/base/VERSION.ini
    Should contain   ${base_version_contents}   PRODUCT_VERSION = 99.9.9
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

    #Log SSPLAV logs to the test report
    Log File      ${AV_LOG_FILE}
    Log File      ${THREAT_DETECTOR_LOG_PATH}

    Wait For Suldownloader To Finish
    Mark Known Upgrade Errors
    # Specific to this test:
    #TODO LINUXDAR-3187 remove when this defect is closed
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/watchdog.log  ProcessMonitoringImpl <> /opt/sophos-spl/plugins/av/sbin/sophos_threat_detector_launcher died with 15
    #TODO LINUXDAR-3188 remove when this defect is closed
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/av.log  UnixSocket <> Failed to write Process Control Request to socket. Exception caught: Environment interruption
    #TODO LINUXDAR-3191 remove when this defect is closed
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/av.log  av <> Failed to get SAV policy at startup (No Policy Available)
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/av.log  av <> Failed to get ALC policy at startup (No Policy Available)

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical

Install master of base and edr and mtr and upgrade to base 999
    Install EDR  ${BaseAndEdrAndMtrVUTPolicy}

    Check log Does not Contain   Installing product: ServerProtectionLinux-Base-component version: 99.9.9   ${SULDOWNLOADER_LOG_PATH}  Sul-Downloader

    Send ALC Policy And Prepare For Upgrade  ${Base999Policy}
    #truncate log so that check mdr plugin installed works correctly later in the test
    ${result} =  Run Process   truncate   -s   0   ${MTR_DIR}/log/mtr.log

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check SulDownloader Log Contains     Installing product: ServerProtectionLinux-Base-component version: 99.9.9
    Wait Until Keyword Succeeds
    ...   90 secs
    ...   5 secs
    ...   File Should exist   ${SOPHOS_INSTALL}/base/update/var/updatescheduler/upgrade_marker_file
    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   File Should not exist   ${SOPHOS_INSTALL}/base/update/var/updatescheduler/upgrade_marker_file
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
    Start Local Cloud Server  --initial-alc-policy  ${BaseAndEdrVUTPolicy}
    ${statusPath}=  Set Variable  ${MCS_DIR}/status/ALC_status.xml
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseAndEdrVUTPolicy}

    # Install EDR
    Send ALC Policy And Prepare For Upgrade  ${BaseAndEdrVUTPolicy}

    Wait for first update
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
    ...  20 secs
    ...  1 secs
    ...   Check Log Contains String N times   ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Update success  2

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
    Start Local Cloud Server  --initial-alc-policy  ${BaseAndEdrAndMtrVUTPolicy}
    Log File  /etc/hosts
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseAndEdrAndMtrVUTPolicy}
    Override Local LogConf File Using Content  [global]\nVERBOSITY = DEBUG\n
    # Install EDR And MTR
    Send ALC Policy And Prepare For Upgrade  ${BaseAndEdrAndMtrVUTPolicy}

    Wait for first update

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
    ...  Check Log Contains String N times   ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Update success  2

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
    ${string} =  Check Log And Return Nth Occurence Between Strings  <ns:events   </ns:events>   ${SOPHOS_INSTALL}/logs/base/sophosspl/mcs_envelope.log   ${Event_Number}
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
    ${mtr_vut_version} =  get_version_for_rigidname_in_vut_warehouse   ServerProtectionLinux-MDR-Control-Component
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

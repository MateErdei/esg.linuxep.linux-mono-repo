*** Settings ***
Library    OperatingSystem

Library     ${LIB_FILES}/TeardownTools.py
Library     ${LIB_FILES}/LogUtils.py

Resource    ProductResources.robot

*** Keywords ***
Dump All Logs
    dump_thininstaller_log
    dump_cloud_server_log

    dump_teardown_log    ${SOPHOS_INSTALL}/logs/installation/ServerProtectionLinux-Base-component_install.log
    dump_teardown_log    ${SOPHOS_INSTALL}/logs/installation/ServerProtectionLinux-Plugin-AV_install.log
    dump_teardown_log    ${SOPHOS_INSTALL}/logs/installation/ServerProtectionLinux-Plugin-EDR_install.log
    dump_teardown_log    ${SOPHOS_INSTALL}/logs/installation/ServerProtectionLinux-Plugin-EventJournaler_install.log
    dump_teardown_log    ${SOPHOS_INSTALL}/logs/installation/ServerProtectionLinux-Plugin-RuntimeDetections_install.log
    dump_teardown_log    ${SOPHOS_INSTALL}/logs/installation/ServerProtectionLinux-Plugin-liveresponse_install.log
    dump_teardown_log    ${SOPHOS_INSTALL}/logs/installation/ServerProtectionLinux-Plugin-responseactions_install.log

    dump_teardown_log    ${BASE_LOGS_DIR}/downgrade-backup/sophosspl/updatescheduler.log

    dump_teardown_log    ${BASE_LOGS_DIR}/downgrade-backup/suldownloader.log
    dump_teardown_log    ${BASE_LOGS_DIR}/register_central.log
    dump_teardown_log    ${BASE_LOGS_DIR}/suldownloader.log
    dump_teardown_log    ${BASE_LOGS_DIR}/suldownloader_sync.log
    dump_teardown_log    ${BASE_LOGS_DIR}/watchdog.log
    dump_teardown_log    ${BASE_LOGS_DIR}/wdctl.log

    dump_teardown_log    ${BASE_LOGS_DIR}/sophosspl/mcs_envelope.log
    dump_teardown_log    ${BASE_LOGS_DIR}/sophosspl/mcsrouter.log
    dump_teardown_log    ${BASE_LOGS_DIR}/sophosspl/remote_diagnose.log
    dump_teardown_log    ${BASE_LOGS_DIR}/sophosspl/sophos_managementagent.log
    dump_teardown_log    ${BASE_LOGS_DIR}/sophosspl/telemetry.log
    dump_teardown_log    ${BASE_LOGS_DIR}/sophosspl/tscheduler.log
    dump_teardown_log    ${BASE_LOGS_DIR}/sophosspl/updatescheduler.log

    dump_teardown_log    ${SOPHOS_INSTALL}/base/update/var/updatescheduler/update_config.json
    dump_teardown_log    ${MCS_DIR}/policy/ALC-1_policy.xml
    dump_teardown_log    ${MCS_DIR}/policy/CORE_policy.xml
    dump_teardown_log    ${MCS_DIR}/policy/CORC_policy.xml
    dump_teardown_log    ${MCS_DIR}/policy/SAV-2_policy.xml
    dump_teardown_log    ${MCS_DIR}/policy/flags.json

    dump_teardown_log    ${SOPHOS_INSTALL}/base/etc/logger.conf
    dump_teardown_log    ${BASE_LOGS_DIR}/etc/DiagnosePaths.conf

    dump_teardown_log    ${EDR_DIR}/log/edr.log
    dump_teardown_log    ${EDR_DIR}/log/scheduledquery.log
    dump_teardown_log    ${EDR_DIR}/log/edr_osquery.log
    dump_teardown_log    ${EDR_DIR}/log/livequery.log
    dump_teardown_log    ${EDR_DIR}/log/SophosMTRExtension.log
    dump_teardown_log    ${EDR_DIR}/log/osqueryd.INFO
    dump_teardown_log    ${EDR_DIR}/log/osqueryd.results.log
    dump_teardown_log    ${EDR_DIR}/etc/osquery.conf.d/sophos-scheduled-query-pack.mtr.conf
    dump_teardown_log    ${EDR_DIR}/etc/osquery.conf.d/sophos-scheduled-query-pack.conf

    dump_teardown_log    ${LIVERESPONSE_DIR}/log/liveresponse.log
    dump_teardown_log    ${LIVERESPONSE_DIR}/log/sessions.log

    dump_teardown_log    ${AV_DIR}/log/av.log
    dump_teardown_log    ${AV_DIR}/log/soapd.log
    dump_teardown_log    ${AV_DIR}/log/sophos_threat_detector/sophos_threat_detector.log
    dump_teardown_log    ${AV_DIR}/log/safestore.log

    dump_teardown_log    ${EVENTJOURNALER_DIR}/log/eventjournaler.log

    dump_teardown_log    ${RESPONSE_ACTIONS_DIR}/log/responseactions.log
    dump_teardown_log    ${RESPONSE_ACTIONS_DIR}/log/actionrunner.log

    dump_teardown_log    ${RTD_DIR}/log/runtimedetections.log

    dump_teardown_log    ./tmp/proxy_server.log
    dump_teardown_log    /tmp/sdds3_server.log
    dump_teardown_log    ./tmp/proxy.log
    dump_teardown_log    ./tmp/relay.log
    dump_teardown_log    ./tmp/CapnPublisher.log
    dump_teardown_log    ./tmp/update_server.log
    dump_teardown_log    ./tmp/CapnSubscriber.log
    dump_teardown_log    ./tmp/push_server_log.log

    dump_teardown_log    ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Base-component/addedFiles_manifest.dat
    dump_teardown_log    ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Base-component/removedFiles_manifest.dat
    dump_teardown_log    ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Base-component/changedFiles_manifest.dat

    dump_teardown_log    ${SOPHOS_INSTALL}/base/VERSION.ini
    dump_teardown_log    ${AV_DIR}/VERSION.ini
    dump_teardown_log    ${EDR_DIR}/VERSION.ini
    dump_teardown_log    ${EVENTJOURNALER_DIR}/VERSION.ini
    dump_teardown_log    ${LIVERESPONSE_DIR}/VERSION.ini
    dump_teardown_log    ${RESPONSE_ACTIONS_DIR}/VERSION.ini
    dump_teardown_log    ${RTD_DIR}/VERSION.ini

Dump All Sophos Processes
    ${result}=  Run Process    ps -elf | grep sophos    shell=True
    Log  ${result.stdout}

Log Status Of Sophos Spl
    ${result} =  Run Process    systemctl  status  sophos-spl
    Log  ${result.stdout}
    ${result} =  Run Process    systemctl  status  sophos-spl-update
    Log  ${result.stdout}
    ${result} =  Run Process    systemctl  status  sophos-spl-diagnose
    Log  ${result.stdout}

Log Systemctl Timers
    ${result} =  Run Process    systemctl  list-timers
    Log  ${result.stdout}

Log Status Of Rsyslog
    ${result} =  Run Process    systemctl  status  rsyslog
    Log  ${result.stdout}
    Log  ${result.stderr}

Check Journalctl
    analyse_Journalctl   print_always=False

Check and Dump Journalctl
    analyse_Journalctl   print_always=True

General Test Teardown
    require_no_unhandled_exception
    check_dmesg_for_segfaults
    Run Keyword If Test Failed    Dump All Logs
    Remove File                   /tmp/sdds3_server.log
    Run Keyword If Test Failed    Check and Dump Journalctl
    Run Keyword If Test Passed    Check Journalctl
    Run Keyword If Test Failed    Log Status Of Rsyslog
    Run Keyword If Test Failed    Log Systemctl Timers
    Run Keyword If Test Failed    Log Status Of Sophos Spl
    Run Keyword If Test Failed    Display All SSPL Files Installed
    Run Keyword If Test Failed    Display All SSPL Plugins Files Installed
    Run Keyword If Test Failed    Dump All Sophos Processes
    combine_coverage_if_present

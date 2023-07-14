*** Settings ***
Library    OperatingSystem

Library     ${LIB_FILES}/TeardownTools.py
Library     ${LIB_FILES}/LogUtils.py

Resource    ProductResources.robot

*** Keywords ***
Dump All Logs
    Dump Thininstaller Log
    Dump Cloud Server Log

    Dump Teardown Log    ${BASE_LOGS_DIR}/downgrade-backup/sophosspl/updatescheduler.log

    Dump Teardown Log    ${BASE_LOGS_DIR}/downgrade-backup/suldownloader.log
    Dump Teardown Log    ${BASE_LOGS_DIR}/register_central.log
    Dump Teardown Log    ${BASE_LOGS_DIR}/suldownloader.log
    Dump Teardown Log    ${BASE_LOGS_DIR}/suldownloader_sync.log
    Dump Teardown Log    ${BASE_LOGS_DIR}/watchdog.log
    Dump Teardown Log    ${BASE_LOGS_DIR}/wdctl.log

    Dump Teardown Log    ${BASE_LOGS_DIR}/sophosspl/mcs_envelope.log
    Dump Teardown Log    ${BASE_LOGS_DIR}/sophosspl/mcsrouter.log
    Dump Teardown Log    ${BASE_LOGS_DIR}/sophosspl/remote_diagnose.log
    Dump Teardown Log    ${BASE_LOGS_DIR}/sophosspl/sophos_managementagent.log
    Dump Teardown Log    ${BASE_LOGS_DIR}/sophosspl/telemetry.log
    Dump Teardown Log    ${BASE_LOGS_DIR}/sophosspl/tscheduler.log
    Dump Teardown Log    ${BASE_LOGS_DIR}/sophosspl/updatescheduler.log

    Dump Teardown Log    ${SOPHOS_INSTALL}/base/update/var/updatescheduler/update_config.json
    Dump Teardown Log    ${MCS_DIR}/policy/ALC-1_policy.xml
    Dump Teardown Log    ${MCS_DIR}/policy/CORE_policy.xml
    Dump Teardown Log    ${MCS_DIR}/policy/CORC_policy.xml
    Dump Teardown Log    ${MCS_DIR}/policy/SAV-2_policy.xml
    Dump Teardown Log    ${MCS_DIR}/policy/flags.json

    Dump Teardown Log    ${SOPHOS_INSTALL}/base/etc/logger.conf
    Dump Teardown Log    ${BASE_LOGS_DIR}/etc/DiagnosePaths.conf

    Dump Teardown Log    ${EDR_DIR}/log/edr.log
    Dump Teardown Log    ${EDR_DIR}/log/scheduledquery.log
    Dump Teardown Log    ${EDR_DIR}/log/edr_osquery.log
    Dump Teardown Log    ${EDR_DIR}/log/livequery.log
    Dump Teardown Log    ${EDR_DIR}/log/SophosMTRExtension.log
    Dump Teardown Log    ${EDR_DIR}/log/osqueryd.INFO
    Dump Teardown Log    ${EDR_DIR}/log/osqueryd.results.log
    Dump Teardown Log    ${EDR_DIR}/etc/osquery.conf.d/sophos-scheduled-query-pack.mtr.conf
    Dump Teardown Log    ${EDR_DIR}/etc/osquery.conf.d/sophos-scheduled-query-pack.conf

    Dump Teardown Log    ${LIVERESPONSE_DIR}/log/liveresponse.log
    Dump Teardown Log    ${LIVERESPONSE_DIR}/log/sessions.log

    Dump Teardown Log    ${AV_DIR}/log/av.log
    Dump Teardown Log    ${AV_DIR}/log/soapd.log
    Dump Teardown Log    ${AV_DIR}/log/sophos_threat_detector/sophos_threat_detector.log
    Dump Teardown Log    ${AV_DIR}/log/safestore.log

    Dump Teardown Log    ${EVENTJOURNALER_DIR}/log/eventjournaler.log

    Dump Teardown Log    ${RESPONSE_ACTIONS_DIR}/log/responseactions.log
    Dump Teardown Log    ${RESPONSE_ACTIONS_DIR}/log/actionrunner.log

    Dump Teardown Log    ${RTD_DIR}/log/runtimedetections.log

    Dump Teardown Log    ./tmp/proxy_server.log
    Dump Teardown Log    /tmp/sdds3_server.log
    Dump Teardown Log    ./tmp/proxy.log
    Dump Teardown Log    ./tmp/relay.log
    Dump Teardown Log    ./tmp/CapnPublisher.log
    Dump Teardown Log    ./tmp/update_server.log
    Dump Teardown Log    ./tmp/CapnSubscriber.log
    Dump Teardown Log    ./tmp/push_server_log.log

    Dump Teardown Log    ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Base-component/addedFiles_manifest.dat
    Dump Teardown Log    ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Base-component/removedFiles_manifest.dat
    Dump Teardown Log    ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Base-component/changedFiles_manifest.dat

    Dump Teardown Log    ${SOPHOS_INSTALL}/base/VERSION.ini
    Dump Teardown Log    ${AV_DIR}/VERSION.ini
    Dump Teardown Log    ${EDR_DIR}/VERSION.ini
    Dump Teardown Log    ${EVENTJOURNALER_DIR}/VERSION.ini
    Dump Teardown Log    ${LIVERESPONSE_DIR}/VERSION.ini
    Dump Teardown Log    ${RESPONSE_ACTIONS_DIR}/VERSION.ini
    Dump Teardown Log    ${RTD_DIR}/VERSION.ini

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
    Analyse Journalctl   print_always=False

Check and Dump Journalctl
    Analyse Journalctl   print_always=True

General Test Teardown
    Require No Unhandled Exception
    Check Dmesg For Segfaults
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
    Force Teardown Logging If Env Set

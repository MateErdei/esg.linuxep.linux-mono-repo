*** Settings ***
Library     ../libs/TeardownTools.py

Resource    installer/InstallerResources.robot
Library    OperatingSystem

*** Variables ***

*** Keywords ***
Dump All Logs
    # Old location
    Dump Teardown Log    ${SOPHOS_INSTALL}/logs/base/updatescheduler.log
    # new location
    Dump Teardown Log    ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log
    Dump Teardown Log    ${SOPHOS_INSTALL}/logs/base/sophosspl/remote_diagnose.log
    Dump Teardown Log    ${SOPHOS_INSTALL}/logs/base/downgrade-backup/sophosspl/updatescheduler.log
    Dump Teardown Log    ${SOPHOS_INSTALL}/logs/base/wdctl.log
    Dump Teardown Log    ${SOPHOS_INSTALL}/logs/base/watchdog.log

    Dump Teardown Log    ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    Dump Teardown Log    ${SOPHOS_INSTALL}/logs/base/suldownloader_sync.log
    Dump Teardown Log    ${SOPHOS_INSTALL}/logs/base/downgrade-backup/suldownloader.log
    Dump Teardown Log    ${SOPHOS_INSTALL}/logs/base/register_central.log

    Dump Teardown Log    ${SOPHOS_INSTALL}/logs/base/comms_startup.log
    Dump Teardown Log    ${SOPHOS_INSTALL}/logs/base/sophosspl/comms_component.log
    Dump Teardown Log    ${SOPHOS_INSTALL}/logs/base/sophos-spl-comms/comms_network.log
    Dump Teardown Log    ${SOPHOS_INSTALL}/logs/base/sophosspl/mcs_envelope.log
    Dump Teardown Log    ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log
    Dump Teardown Log    ${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log
    Dump Teardown Log    ${SOPHOS_INSTALL}/logs/base/sophosspl/telemetry.log
    Dump Teardown Log    ${SOPHOS_INSTALL}/logs/base/sophosspl/tscheduler.log
    Dump Teardown Log    ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log

    Dump Teardown Log    ${UPDATE_CONFIG}
    Dump Teardown Log    ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml

    Dump Teardown Log    ${SOPHOS_INSTALL}/base/etc/logger.conf
    Dump Teardown Log    ${SOPHOS_INSTALL}/logs/base/etc/DiagnosePaths.conf

    Dump Teardown Log    ${SOPHOS_INSTALL}/plugins/edr/log/edr.log
    Dump Teardown Log    ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log
    Dump Teardown Log    ${SOPHOS_INSTALL}/plugins/edr/log/edr_osquery.log
    Dump Teardown Log    ${SOPHOS_INSTALL}/plugins/edr/log/livequery.log
    Dump Teardown Log    ${SOPHOS_INSTALL}/plugins/edr/log/SophosMTRExtension.log
    Dump Teardown Log    ${SOPHOS_INSTALL}/plugins/edr/log/osqueryd.INFO
    Dump Teardown Log    ${SOPHOS_INSTALL}/plugins/edr/log/osqueryd.results.log
    Dump Teardown Log    ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.mtr.conf
    Dump Teardown Log    ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf

    Dump Teardown Log    ${LIVERESPONSE_DIR}/log/liveresponse.log
    Dump Teardown Log    ${LIVERESPONSE_DIR}/log/sessions.log

    Dump Teardown Log    ${SOPHOS_INSTALL}/plugins/mtr/log/mtr.log
    Dump Teardown Log    ${SOPHOS_INSTALL}/plugins/mtr/dbos/data/logs/dbos.log
    Dump Teardown Log    ${SOPHOS_INSTALL}/plugins/mtr/dbos/data/logs/osquery.watcher.log

    Dump Teardown Log    ${SOPHOS_INSTALL}/plugins/av/log/av.log
    Dump Teardown Log    ${SOPHOS_INSTALL}/plugins/av/log/sophos_threat_detector/sophos_threat_detector.log

    Dump Teardown Log    ${SOPHOS_INSTALL}/plugins/eventjournaler/log/eventjournaler.log

    Dump Teardown Log    ${SOPHOS_INSTALL}/plugins/runtimedetections/log/runtimedetections.log
    Dump Teardown Log    ${SOPHOS_INSTALL}/plugins/heartbeat/log/connection_tracker.log
    Dump Teardown Log    ${SOPHOS_INSTALL}/plugins/heartbeat/log/heartbeat.log

    Dump Teardown Log    ./tmp/proxy_server.log
    Dump Teardown Log    /tmp/sdds3_server.log
    Remove File          /tmp/sdds3_server.log
    Dump Teardown Log    ./tmp/proxy.log
    Dump Teardown Log    ./tmp/relay.log
    Dump Teardown Log    ./tmp/CapnPublisher.log
    Dump Teardown Log    ./tmp/update_server.log
    Dump Teardown Log    ./tmp/CapnSubscriber.log
    Dump Teardown Log    ./tmp/warehouseGenerator.log

    Dump Teardown Log    ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Base-component/addedFiles_manifest.dat
    Dump Teardown Log    ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Base-component/removedFiles_manifest.dat
    Dump Teardown Log    ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Base-component/changedFiles_manifest.dat

    Dump Teardown Log    ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Plugin-MDR/addedFiles_manifest.dat
    Dump Teardown Log    ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Plugin-MDR/changedFiles_manifest.dat
    Dump Teardown Log    ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Plugin-MDR/removedFiles_manifest.dat

    Dump Teardown Log    ${SOPHOS_INSTALL}/base/VERSION.ini
    Dump Teardown Log    ${SOPHOS_INSTALL}/plugins/mtr/VERSION.ini
    Dump Teardown Log    ${SOPHOS_INSTALL}/plugins/liveresponse/VERSION.ini
    Dump Teardown Log    ${SOPHOS_INSTALL}/plugins/edr/VERSION.ini
    Dump Teardown Log    ${SOPHOS_INSTALL}/plugins/av/VERSION.ini

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
    Check For Coredumps  ${TEST NAME}
    Check Dmesg For Segfaults
    Run Keyword If Test Failed    Dump All Logs
    Run Keyword If Test Failed    Check and Dump Journalctl
    Run Keyword If Test Passed    Check Journalctl
    Run Keyword If Test Failed    Log Status Of Rsyslog
    Run Keyword If Test Failed    Log Systemctl Timers
    Run Keyword If Test Failed    Log Status Of Sophos Spl
    Run Keyword If Test Failed    Display All SSPL Files Installed
    Run Keyword If Test Failed    Display All SSPL Plugins Files Installed
    Run Keyword If Test Failed    Dump All Sophos Processes
    Force Teardown Logging If Env Set
    Combine Coverage If Present

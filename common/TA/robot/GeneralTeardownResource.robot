*** Settings ***
Library    OperatingSystem
Library    TeardownTools.py
Library    LogUtils.py

Resource   InstallerResources.robot

*** Variables ***

*** Keywords ***
Dump All Logs
    [Arguments]    ${installDir}=${SOPHOS_INSTALL}
    Dump Thininstaller Log
    Dump Cloud Server Log
    
    Dump Teardown Log    ${installDir}/logs/base/sophosspl/updatescheduler.log

    Dump Teardown Log    ${installDir}/logs/base/wdctl.log
    Dump Teardown Log    ${installDir}/logs/base/watchdog.log

    Dump Teardown Log    ${installDir}/logs/base/suldownloader.log
    Dump Teardown Log    ${installDir}/logs/base/suldownloader_sync.log
    Dump Teardown Log    ${installDir}/logs/base/register_central.log

    Dump Teardown Log    ${installDir}/logs/base/sophosspl/mcs_envelope.log
    Dump Teardown Log    ${installDir}/logs/base/sophosspl/mcsrouter.log
    Dump Teardown Log    ${installDir}/logs/base/sophosspl/sophos_managementagent.log
    Dump Teardown Log    ${installDir}/logs/base/sophosspl/telemetry.log
    Dump Teardown Log    ${installDir}/logs/base/sophosspl/tscheduler.log
    Dump Teardown Log    ${installDir}/logs/base/sophosspl/remote_diagnose.log

    Dump Teardown Log    ${installDir}/logs/installation/ServerProtectionLinux-Base-component_install.log
    Dump Teardown Log    ${installDir}/logs/installation/ServerProtectionLinux-Plugin-AV_install.log
    Dump Teardown Log    ${installDir}/logs/installation/ServerProtectionLinux-Plugin-EDR_install.log
    Dump Teardown Log    ${installDir}/logs/installation/ServerProtectionLinux-Plugin-EventJournaler_install.log
    Dump Teardown Log    ${installDir}/logs/installation/ServerProtectionLinux-Plugin-RuntimeDetections_install.log
    Dump Teardown Log    ${installDir}/logs/installation/ServerProtectionLinux-Plugin-liveresponse_install.log
    Dump Teardown Log    ${installDir}/logs/installation/ServerProtectionLinux-Plugin-responseactions_install.log

    Dump Teardown Log    ${installDir}/base/update/var/updatescheduler/update_config.json
    Dump Teardown Log    ${installDir}/base/mcs/policy/ALC-1_policy.xml
    Dump Teardown Log    ${installDir}/base/mcs/policy/CORE_policy.xml
    Dump Teardown Log    ${installDir}/base/mcs/policy/SAV-2_policy.xml
    Dump Teardown Log    ${installDir}/base/mcs/policy/flags.json

    Dump Teardown Log    ${installDir}/base/etc/logger.conf
    Dump Teardown Log    ${installDir}/logs/base/etc/DiagnosePaths.conf

    Dump Teardown Log    ${installDir}/plugins/edr/log/edr.log
    Dump Teardown Log    ${installDir}/plugins/edr/log/scheduledquery.log
    Dump Teardown Log    ${installDir}/plugins/edr/log/edr_osquery.log
    Dump Teardown Log    ${installDir}/plugins/edr/log/livequery.log
    Dump Teardown Log    ${installDir}/plugins/edr/log/SophosMTRExtension.log
    Dump Teardown Log    ${installDir}/plugins/edr/log/osqueryd.INFO
    Dump Teardown Log    ${installDir}/plugins/edr/log/osqueryd.results.log
    Dump Teardown Log    ${installDir}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.mtr.conf
    Dump Teardown Log    ${installDir}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf

    Dump Teardown Log    ${installDir}/plugins/liveresponse/log/liveresponse.log
    Dump Teardown Log    ${installDir}/plugins/liveresponse/log/sessions.log

    Dump Teardown Log    ${installDir}/plugins/av/log/av.log
    Dump Teardown Log    ${installDir}/plugins/av/log/soapd.log
    Dump Teardown Log    ${installDir}/plugins/av/log/sophos_threat_detector/sophos_threat_detector.log
    Dump Teardown Log    ${installDir}/plugins/av/log/safestore.log

    Dump Teardown Log    ${installDir}/plugins/eventjournaler/log/eventjournaler.log

    Dump Teardown Log    ${installDir}/plugins/responseactions/log/responseactions.log
    Dump Teardown Log    ${installDir}/plugins/responseactions/log/actionrunner.log

    Dump Teardown Log    ${installDir}/plugins/runtimedetections/log/runtimedetections.log
    Dump Teardown Log    ${installDir}/plugins/heartbeat/log/connection_tracker.log
    Dump Teardown Log    ${installDir}/plugins/heartbeat/log/heartbeat.log

    Dump Teardown Log    ./tmp/proxy_server.log
    Dump Teardown Log    /tmp/sdds3_server.log
    Dump Teardown Log    ./tmp/proxy.log
    Dump Teardown Log    ./tmp/relay.log
    Dump Teardown Log    ./tmp/CapnPublisher.log
    Dump Teardown Log    ./tmp/update_server.log
    Dump Teardown Log    ./tmp/CapnSubscriber.log
    Dump Teardown Log    ./tmp/push_server_log.log

    Dump Teardown Log    ${installDir}/tmp/ServerProtectionLinux-Base-component/addedFiles_manifest.dat
    Dump Teardown Log    ${installDir}/tmp/ServerProtectionLinux-Base-component/removedFiles_manifest.dat
    Dump Teardown Log    ${installDir}/tmp/ServerProtectionLinux-Base-component/changedFiles_manifest.dat


    Dump Teardown Log    ${installDir}/base/VERSION.ini
    Dump Teardown Log    ${installDir}/plugins/av/VERSION.ini
    Dump Teardown Log    ${installDir}/plugins/edr/VERSION.ini
    Dump Teardown Log    ${installDir}/plugins/eventjournaler/VERSION.ini
    Dump Teardown Log    ${installDir}/plugins/liveresponse/VERSION.ini
    Dump Teardown Log    ${installDir}/plugins/responseactions/VERSION.ini
    Dump Teardown Log    ${installDir}/plugins/runtimedetections/VERSION.ini

    Dump Teardown Log    ${installDir}/base/etc/user-group-ids-actual.conf
    Dump Teardown Log    ${installDir}/base/etc/user-group-ids-requested.conf
    Dump Teardown Log    ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}/http_test_server.log

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
    [Arguments]    ${installDir}=${SOPHOS_INSTALL}
    Require No Unhandled Exception
    Check For Coredumps  ${TEST NAME}
    Check Dmesg For Segfaults
    Run Keyword If Test Failed    Dump All Logs    ${installDir}
    Remove File                   /tmp/sdds3_server.log
    Run Keyword If Test Failed    Check and Dump Journalctl
    Run Keyword If Test Passed    Check Journalctl
    Run Keyword If Test Failed    Log Status Of Rsyslog
    Run Keyword If Test Failed    Log Systemctl Timers
    Run Keyword If Test Failed    Log Status Of Sophos Spl
    Run Keyword If Test Failed    Display All SSPL Files Installed    ${installDir}
    Run Keyword If Test Failed    Display All SSPL Plugins Files Installed    ${installDir}
    Run Keyword If Test Failed    Dump All Sophos Processes
    Force Teardown Logging If Env Set
    Combine Coverage If Present

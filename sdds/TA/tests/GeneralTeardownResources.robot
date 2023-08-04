*** Settings ***
Library    OperatingSystem

Library     ${LIB_FILES}/TeardownTools.py
Library     ${LIB_FILES}/LogUtils.py

Resource    ProductResources.robot

*** Keywords ***
Dump All Logs
    [Arguments]    ${installDir}=${SOPHOS_INSTALL}
    dump_thininstaller_log
    dump_cloud_server_log

    dump_teardown_log    ${installDir}/logs/installation/ServerProtectionLinux-Base-component_install.log
    dump_teardown_log    ${installDir}/logs/installation/ServerProtectionLinux-Plugin-AV_install.log
    dump_teardown_log    ${installDir}/logs/installation/ServerProtectionLinux-Plugin-EDR_install.log
    dump_teardown_log    ${installDir}/logs/installation/ServerProtectionLinux-Plugin-EventJournaler_install.log
    dump_teardown_log    ${installDir}/logs/installation/ServerProtectionLinux-Plugin-RuntimeDetections_install.log
    dump_teardown_log    ${installDir}/logs/installation/ServerProtectionLinux-Plugin-liveresponse_install.log
    dump_teardown_log    ${installDir}/logs/installation/ServerProtectionLinux-Plugin-responseactions_install.log

    dump_teardown_log    ${installDir}/logs/base/downgrade-backup/sophosspl/updatescheduler.log

    dump_teardown_log    ${installDir}/logs/base/downgrade-backup/suldownloader.log
    dump_teardown_log    ${installDir}/logs/base/register_central.log
    dump_teardown_log    ${installDir}/logs/base/suldownloader.log
    dump_teardown_log    ${installDir}/logs/base/suldownloader_sync.log
    dump_teardown_log    ${installDir}/logs/base/watchdog.log
    dump_teardown_log    ${installDir}/logs/base/wdctl.log

    dump_teardown_log    ${installDir}/logs/base/sophosspl/mcs_envelope.log
    dump_teardown_log    ${installDir}/logs/base/sophosspl/mcsrouter.log
    dump_teardown_log    ${installDir}/logs/base/sophosspl/remote_diagnose.log
    dump_teardown_log    ${installDir}/logs/base/sophosspl/sophos_managementagent.log
    dump_teardown_log    ${installDir}/logs/base/sophosspl/telemetry.log
    dump_teardown_log    ${installDir}/logs/base/sophosspl/tscheduler.log
    dump_teardown_log    ${installDir}/logs/base/sophosspl/updatescheduler.log

    dump_teardown_log    ${installDir}/base/update/var/updatescheduler/update_config.json
    dump_teardown_log    ${installDir}/base/mcs/policy/ALC-1_policy.xml
    dump_teardown_log    ${installDir}/base/mcs/policy/CORE_policy.xml
    dump_teardown_log    ${installDir}/base/mcs/policy/CORC_policy.xml
    dump_teardown_log    ${installDir}/base/mcs/policy/SAV-2_policy.xml
    dump_teardown_log    ${installDir}/base/mcs/policy/flags.json

    dump_teardown_log    ${installDir}/base/etc/logger.conf
    dump_teardown_log    ${installDir}/logs/base/etc/DiagnosePaths.conf

    dump_teardown_log    ${installDir}/plugins/edr/log/edr.log
    dump_teardown_log    ${installDir}/plugins/edr/log/scheduledquery.log
    dump_teardown_log    ${installDir}/plugins/edr/log/edr_osquery.log
    dump_teardown_log    ${installDir}/plugins/edr/log/livequery.log
    dump_teardown_log    ${installDir}/plugins/edr/log/SophosMTRExtension.log
    dump_teardown_log    ${installDir}/plugins/edr/log/osqueryd.INFO
    dump_teardown_log    ${installDir}/plugins/edr/log/osqueryd.results.log
    dump_teardown_log    ${installDir}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.mtr.conf
    dump_teardown_log    ${installDir}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf

    dump_teardown_log    ${installDir}/plugins/liveresponse/log/liveresponse.log
    dump_teardown_log    ${installDir}/plugins/liveresponse/log/sessions.log

    dump_teardown_log    ${installDir}/plugins/av/log/av.log
    dump_teardown_log    ${installDir}/plugins/av/log/soapd.log
    dump_teardown_log    ${installDir}/plugins/av/log/sophos_threat_detector/sophos_threat_detector.log
    dump_teardown_log    ${installDir}/plugins/av/log/safestore.log

    dump_teardown_log    ${installDir}/plugins/eventjournaler/log/eventjournaler.log

    dump_teardown_log    ${installDir}/plugins/responseactions/log/responseactions.log
    dump_teardown_log    ${installDir}/plugins/responseactions/log/actionrunner.log

    dump_teardown_log    ${installDir}/plugins/rtd/log/runtimedetections.log

    dump_teardown_log    ./tmp/proxy_server.log
    dump_teardown_log    ./tmp/sdds3_server.log
    dump_teardown_log    ./tmp/proxy.log
    dump_teardown_log    ./tmp/relay.log
    dump_teardown_log    ./tmp/CapnPublisher.log
    dump_teardown_log    ./tmp/update_server.log
    dump_teardown_log    ./tmp/CapnSubscriber.log
    dump_teardown_log    ./tmp/push_server_log.log

    dump_teardown_log    ${installDir}/tmp/ServerProtectionLinux-Base-component/addedFiles_manifest.dat
    dump_teardown_log    ${installDir}/tmp/ServerProtectionLinux-Base-component/removedFiles_manifest.dat
    dump_teardown_log    ${installDir}/tmp/ServerProtectionLinux-Base-component/changedFiles_manifest.dat

    dump_teardown_log    ${installDir}/base/VERSION.ini
    dump_teardown_log    ${installDir}/plugins/av/VERSION.ini
    dump_teardown_log    ${installDir}/plugins/edr/VERSION.ini
    dump_teardown_log    ${installDir}/plugins/eventjournaler/VERSION.ini
    dump_teardown_log    ${installDir}/plugins/liveresponse/VERSION.ini
    dump_teardown_log    ${installDir}/plugins/responseactions/VERSION.ini
    dump_teardown_log    ${installDir}/plugins/rtd/VERSION.ini

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
    [Arguments]    ${installDir}=${SOPHOS_INSTALL}
    require_no_unhandled_exception
    check_dmesg_for_segfaults
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
    combine_coverage_if_present

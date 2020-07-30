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
    Dump Teardown Log    ${SOPHOS_INSTALL}/logs/base/wdctl.log
    Dump Teardown Log    ${SOPHOS_INSTALL}/logs/base/watchdog.log

    Dump Teardown Log    ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    Dump Teardown Log    ${SOPHOS_INSTALL}/logs/base/register_central.log

    Dump Teardown Log    ${SOPHOS_INSTALL}/logs/base/sophosspl/comms_component.log
    Dump Teardown Log    ${SOPHOS_INSTALL}/logs/base/sophosspl/mcs_envelope.log
    Dump Teardown Log    ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log
    Dump Teardown Log    ${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log
    Dump Teardown Log    ${SOPHOS_INSTALL}/logs/base/sophosspl/telemetry.log
    Dump Teardown Log    ${SOPHOS_INSTALL}/logs/base/sophosspl/tscheduler.log
    Dump Teardown Log    ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log

    Dump Teardown Log    ${SOPHOS_INSTALL}/base/update/var/update_config.json

    Dump Teardown Log    ${SOPHOS_INSTALL}/logs/base/etc/logger.conf
    Dump Teardown Log    ${SOPHOS_INSTALL}/logs/base/etc/DiagnosePaths.conf

    Dump Teardown Log    ${SOPHOS_INSTALL}/plugins/edr/log/edr.log
    Dump Teardown Log    ${SOPHOS_INSTALL}/plugins/edr/log/livequery.log
    Dump Teardown Log    ${LIVERESPONSE_DIR}/log/liveresponse.log
    Dump Teardown Log    ${LIVERESPONSE_DIR}/log/sessions.log

    Dump Teardown Log    ${SOPHOS_INSTALL}/plugins/mtr/log/mtr.log
    Dump Teardown Log    ${SOPHOS_INSTALL}/plugins/mtr/dbos/data/logs/dbos.log
    Dump Teardown Log    ${SOPHOS_INSTALL}/plugins/mtr/dbos/data/logs/osquery.watcher.log

    Dump Teardown Log    ./tmp/proxy_server.log
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

Dump All Sophos Processes
    ${result}=  Run Process    ps -elf | grep sophos    shell=True
    Log  ${result.stdout}


Log Status Of Sophos Spl
    ${result} =  Run Process    systemctl  status  sophos-spl
    Log  ${result.stdout}
    ${result} =  Run Process    systemctl  status  sophos-spl-update
    Log  ${result.stdout}
    ${result} =  Run Process  ps -ef | grep sophos  shell=True
    Log  ${result.stdout}

Check Journalctl
    Analyse Journalctl   print_always=False

Check and Dump Journalctl
    Analyse Journalctl   print_always=True


General Test Teardown
    Require No Unhandled Exception
    Check For Coredumps  ${TEST NAME}
    Run Keyword If Test Failed    Dump All Logs
    Run Keyword If Test Failed    Check and Dump Journalctl
    Run Keyword If Test Passed    Check Journalctl
    Run Keyword If Test Failed    Log Status Of Sophos Spl
    Run Keyword If Test Failed    Display All SSPL Files Installed
    Run Keyword If Test Failed    Dump All Sophos Processes
    Force Teardown Logging If Env Set
    Check Dmesg For Segfaults
    Combine Coverage If Present

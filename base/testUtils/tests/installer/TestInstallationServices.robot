*** Settings ***
Documentation    Test correct services are started when installer is run

Library    String
Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py

Resource  ../installer/InstallerResources.robot
Resource  ../GeneralTeardownResource.robot

*** Test Cases ***
Verify Watchdog Service Installed And Uninstalled Correctly
    [Tags]  INSTALLER  WATCHDOG  UNINSTALL
    Uninstall SSPL
    Run Full Installer
    ${result} =  Run Process    systemctl    is-active    ${WATCHDOG_SERVICE}
    Should Be Equal  ${result.stdout}    active
    ${result} =  Run Process    systemctl    is-enabled    ${WATCHDOG_SERVICE}
    Should Be Equal  ${result.stdout}    enabled
    ${result} =  Run Process    systemctl status ${WATCHDOG_SERVICE} | grep "Main PID.*sophos_watchdog"  shell=true
    Should Be Equal As Integers  ${result.rc}  0  Unable to get status of ${WATCHDOG_SERVICE} after installation

    Run Process    ${SOPHOS_INSTALL}/bin/uninstall.sh  --force
    File Should Not Exist    /lib/systemd/system/${WATCHDOG_SERVICE}.service
    File Should Not Exist    /usr/lib/systemd/system/${WATCHDOG_SERVICE}.service
    ${result} =  Run Process    systemctl    is-active    ${WATCHDOG_SERVICE}
    Should Be Equal  ${result.stdout}    inactive
    ${result} =  Run Process    systemctl status ${WATCHDOG_SERVICE} | grep "Main PID.*sophos_watchdog"  shell=true
    Should Not Be Equal As Integers  ${result.rc}  0  Able to get status of ${WATCHDOG_SERVICE} after install

Verify Management Agent Running After Installation
    [Tags]  INSTALLER  MANAGEMENT_AGENT
    Uninstall SSPL
    ${result} =  Run Process    pgrep  -f  ${MANAGEMENT_AGENT}
    Log    ${result.stdout}
    Should Not Be Equal As Integers  ${result.rc}  0  Management Agent running after uninstallation
    Run Full Installer
    ${result} =  Run Process    pgrep  -f  ${MANAGEMENT_AGENT}
    Should Be Equal As Integers  ${result.rc}  0  Management Agent not running after installation

    Check Process Running As Sophosspl User And Group  ${result.stdout}

    Run Process    ${SOPHOS_INSTALL}/bin/uninstall.sh  --force
    ${result} =  Run Process    pgrep  -f  ${MANAGEMENT_AGENT}
    Should Not Be Equal As Integers  ${result.rc}  0  Management Agent running after uninstallation

Verify Update Service Installed And Uninstalled Correctly
    [Tags]  INSTALLER  UPDATE_SCHEDULER  UNINSTALL
    Uninstall SSPL
    Run Full Installer
    ${result} =  Run Process    systemctl   status  ${UPDATE_SERVICE}   stderr=STDOUT
    Log    ${result.stdout}
    Should Be Equal As Integers  ${result.rc}  3
    Should Contain    ${result.stdout}    Sophos Server Protection Update Service

    Run Process    ${SOPHOS_INSTALL}/bin/uninstall.sh  --force
    File Should Not Exist    /lib/systemd/system/${UPDATE_SERVICE}.service
    File Should Not Exist    /usr/lib/systemd/system/${UPDATE_SERVICE}.service
    ${result} =  Run Process    systemctl  status  ${UPDATE_SERVICE}   stderr=STDOUT
    Log    ${result.stdout}
    ## Ubuntu 16.04  returns 3 for unknown services
    Should Not Be Equal As Integers  ${result.rc}  0  Able to get status of ${UPDATE_SERVICE} after uninstall.
    Should Contain Any   ${result.stdout}    ${UPDATE_SERVICE}.service could not be found.
    ...    ${UPDATE_SERVICE}.service not found.
    ...    not-found (Reason: No such file or directory)

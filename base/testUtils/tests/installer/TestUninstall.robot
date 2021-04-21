*** Settings ***
Documentation    Test base uninstaller clean up all components

Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py

Resource  ../edr_plugin/EDRResources.robot
Resource  ../mdr_plugin/MDRResources.robot
Resource  ../liveresponse_plugin/LiveResponseResources.robot
Resource  ../GeneralTeardownResource.robot

Default Tags  UNINSTALL

*** Test Cases ***
Uninstallation of base removes all plugins cleanly
    [Tags]  LIVERESPONSE_PLUGIN   EDR_PLUGIN   MDR_PLUGIN  UNINSTALL
    Require Fresh Install

    Check Expected Base Processes Are Running

    Install EDR Directly
    Wait For EDR to be Installed

    Block Connection Between EndPoint And FleetManager
    Install Directly From Component Suite
    Insert MTR Policy
    Check MDR component suite running

    Install Live Response Directly
    Check Live Response Plugin Installed

    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/uninstall.sh  --force

    Should Be Equal As Integers    ${result.rc}    0        uninstaller failed with ${result.rc}

    Check Base Processes Are Not Running
    Check EDR Plugin Uninstalled
    Check MDR Plugin Uninstalled

    Directory Should Not Exist  ${SOPHOS_INSTALL}

Test Liveresponse Plugin uninstalls cleanly
    [Tags]  LIVERESPONSE_PLUGIN  UNINSTALL
    Require Fresh Install

    Install Live Response Directly
    Check Live Response Plugin Installed

    Should Exist  ${SOPHOS_INSTALL}/base/update/var/installedproducts/ServerProtectionLinux-Plugin-liveresponse.sh
    Should Exist  ${SOPHOS_INSTALL}/base/update/var/installedproductversions/ServerProtectionLinux-Plugin-liveresponse.ini
    Should Exist  ${SOPHOS_INSTALL}/var/ipc/plugins/liveresponse.ipc

    ${result} =    Run Process    ${SOPHOS_INSTALL}/base/update/var/installedproducts/ServerProtectionLinux-Plugin-liveresponse.sh

    Should Be Equal As Integers    ${result.rc}    0        uninstaller failed with ${result.rc}

    Should Not Exist  ${SOPHOS_INSTALL}/base/update/var/installedproducts/ServerProtectionLinux-Plugin-liveresponse.sh
    Should Not Exist  ${SOPHOS_INSTALL}/base/update/var/installedproductversions/ServerProtectionLinux-Plugin-liveresponse.ini
    Should Not Exist  ${SOPHOS_INSTALL}/plugins/liveresponse
    Should Not Exist  ${SOPHOS_INSTALL}/var/ipc/plugins/liveresponse.ipc


Test Edr Plugin uninstalls cleanly
    [Tags]  EDR_PLUGIN   UNINSTALL
    Require Fresh Install

    Install EDR Directly
    Wait For EDR to be Installed

    Should Exist  ${SOPHOS_INSTALL}/base/update/var/installedproducts/ServerProtectionLinux-Plugin-EDR.sh
    Should Exist  ${SOPHOS_INSTALL}/base/update/var/installedproductversions/ServerProtectionLinux-Plugin-EDR.ini
    Should Exist  ${SOPHOS_INSTALL}/var/ipc/plugins/edr.ipc


    ${result} =    Run Process    ${SOPHOS_INSTALL}/base/update/var/installedproducts/ServerProtectionLinux-Plugin-EDR.sh

    Should Be Equal As Integers    ${result.rc}    0        uninstaller failed with ${result.rc}

    Should Not Exist  ${SOPHOS_INSTALL}/base/update/var/installedproducts/ServerProtectionLinux-Plugin-EDR.sh
    Should Not Exist  ${SOPHOS_INSTALL}/base/update/var/installedproductversions/ServerProtectionLinux-Plugin-EDR.ini
    Should Not Exist  ${SOPHOS_INSTALL}/plugins/edr
    Should Not Exist  ${SOPHOS_INSTALL}/var/ipc/plugins/edr.ipc
    File Should Not Exist  /etc/rsyslog.d/rsyslog_sophos-spl.conf

Test Edr Plugin downgrades properly with plugin conf
    [Tags]  EDR_PLUGIN   UNINSTALL   PLUGIN_DOWNGRADE
    Require Fresh Install
    Install EDR Directly
    Wait For EDR to be Installed

    Should Exist  ${SOPHOS_INSTALL}/base/update/var/installedproducts/ServerProtectionLinux-Plugin-EDR.sh
    Should Exist  ${SOPHOS_INSTALL}/base/update/var/installedproductversions/ServerProtectionLinux-Plugin-EDR.ini
    Should Exist  ${SOPHOS_INSTALL}/var/ipc/plugins/edr.ipc
    Should Exist  ${SOPHOS_INSTALL}/plugins/edr/etc/plugin.conf

    ${result} =    Run Process    ${SOPHOS_INSTALL}/base/update/var/installedproducts/ServerProtectionLinux-Plugin-EDR.sh   --downgrade  --force

    Should Be Equal As Integers    ${result.rc}    0        uninstaller failed with ${result.rc}

    Should Not Exist  ${SOPHOS_INSTALL}/base/update/var/installedproducts/ServerProtectionLinux-Plugin-EDR.sh
    Should Not Exist  ${SOPHOS_INSTALL}/base/update/var/installedproductversions/ServerProtectionLinux-Plugin-EDR.ini
    Should Exist      ${SOPHOS_INSTALL}/plugins/edr
    Should Not Exist  ${SOPHOS_INSTALL}/var/ipc/plugins/edr.ipc
    ${contents}=  Get File   ${SOPHOS_INSTALL}/plugins/edr/etc/plugin.conf
    Should Contain  ${contents}   running_mode=0

Test Mtr Plugin uninstalls cleanly
    [Tags]  MDR_PLUGIN  UNINSTALL
    Require Fresh Install

    Block Connection Between EndPoint And FleetManager
    Install Directly From Component Suite
    Insert MTR Policy
    Check MDR component suite running

    Should Exist  ${SOPHOS_INSTALL}/base/update/var/installedproducts/ServerProtectionLinux-Plugin-MDR.sh
    Should Exist  ${SOPHOS_INSTALL}/base/update/var/installedproductversions/ServerProtectionLinux-Plugin-MDR.ini
    Should Exist  ${SOPHOS_INSTALL}/var/ipc/plugins/mtr.ipc

    ${result} =    Run Process    ${SOPHOS_INSTALL}/base/update/var/installedproducts/ServerProtectionLinux-Plugin-MDR.sh

    Should Be Equal As Integers    ${result.rc}    0        uninstaller failed with ${result.rc}

    Should Not Exist  ${SOPHOS_INSTALL}/base/update/var/installedproducts/ServerProtectionLinux-Plugin-MDR.sh
    Should Not Exist  ${SOPHOS_INSTALL}/base/update/var/installedproductversions/ServerProtectionLinux-Plugin-MDR.ini
    Should Not Exist  ${SOPHOS_INSTALL}/plugins/mtr
    Should Not Exist  ${SOPHOS_INSTALL}/var/ipc/plugins/mtr.ipc

Test Uninstall Script Gives Return Code Zero
    [Tags]  UNINSTALL  TAP_TESTS  SMOKE
    Require Fresh Install
    Check Expected Base Processes Are Running

    ${result} =  Run Process  ${SOPHOS_INSTALL}/bin/uninstall.sh  --force
    Should Be Equal As Strings  ${result.rc}  0  "Return code was not 0, instead: ${result.rc}\nstdout: ${result.stdout}\nstderr: ${result.stderr}

Test Component Shutdown Cleanly
     Require Fresh Install
     Check Expected Base Processes Are Running

     Install EDR Directly
     Wait For EDR to be Installed

     Install Live Response Directly
     Check Live Response Plugin Installed

     Block Connection Between EndPoint And FleetManager
     Install Directly From Component Suite
     Insert MTR Policy

     Run Process   systemctl  stop  sophos-spl

     Wait Until Keyword Succeeds
     ...  30 secs
     ...  1 secs
     ...  Check EDR Log Contains  Plugin Finished

     Wait Until Keyword Succeeds
     ...  30 secs
     ...  1 secs
     ...  Check Mdr Log Contains   Plugin Finished

     Wait Until Keyword Succeeds
     ...  30 secs
     ...  1 secs
     ...  Check Log Contains   Plugin Finished   ${SOPHOS_INSTALL}/plugins/liveresponse/log/liveresponse.log   LiveResponseLog



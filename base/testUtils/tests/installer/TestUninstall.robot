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
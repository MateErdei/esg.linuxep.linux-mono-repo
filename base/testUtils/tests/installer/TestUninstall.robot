*** Settings ***
Documentation    Test base uninstaller clean up all components

Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py

Resource  ../edr_plugin/EDRResources.robot
Resource  ../mdr_plugin/MDRResources.robot
Resource  ../liveresponse_plugin/LiveResponseResources.robot
Resource  ../GeneralTeardownResource.robot

Default Tags  UNINSTALL  EDR_PLUGIN   EDR_PLUGIN   MDR_PLUGIN

*** Test Cases ***
Uninstallation of base removes all plugins cleanly
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

*** Settings ***
Documentation    Suite of tests to check that MDR is installed and uninstalled correctly based on feature codes and subscriptions in ALC policy.
Resource  MDRResources.robot
Resource  ../scheduler_update/SchedulerUpdateResources.robot
Resource  ../GeneralTeardownResource.robot
Resource  ../upgrade_product/UpgradeResources.robot

Test Setup  Setup Update Scheduler Environment
Test Teardown  Test Teardown

Suite Setup  Setup Ostia Warehouse Environment
Suite Teardown  Teardown Ostia Warehouse Environment

Default Tags   MDR_PLUGIN   MANAGEMENT_AGENT  OSTIA
Force Tags  LOAD4

*** Variables ***
${MDR_PLUGIN_PATH}  ${SOPHOS_INSTALL}/plugins/mtr/
${MDR_VUT_POLICY}   ${SUPPORT_FILES}/CentralXml/RealWarehousePolicies/GeneratedAlcPolicies/base_and_mtr_VUT.xml
${BASE_VUT_POLICY}   ${SUPPORT_FILES}/CentralXml/RealWarehousePolicies/GeneratedAlcPolicies/base_only_VUT.xml


*** Test Cases ***
Test MDR Installed Based On MDR Feature And Subscription In ALC Policy Simple
    [Tags]  SMOKE  MDR_PLUGIN   MANAGEMENT_AGENT  OSTIA  WAREHOUSE_SYNC
    Block Connection Between EndPoint And FleetManager
    Setup Update Scheduler Environment
    Check MDR Plugin uninstalled
    Check SSPL Installed

    # MDR feature and MDR subscription in ALC policy (MDR gets installed)
    Simulate Send Policy And Run Update  ${MDR_VUT_POLICY}
    Check SSPL Installed
    Check MDR Plugin Installed

Test MDR Installed Based On MDR Feature And Subscription In ALC Policy With Combinations
    [Tags]   MDR_PLUGIN   MANAGEMENT_AGENT  OSTIA  WAREHOUSE_SYNC
    Block Connection Between EndPoint And FleetManager
    Check MDR Plugin uninstalled
    Check SSPL Installed

    # MDR feature but no MDR subscription in ALC policy (MDR does not get installed)
    Remove File  ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    Simulate Send Policy And Run Update  ${MDR_VUT_POLICY}  remove_subscriptions=MDR SENSORS
    Check SSPL Installed
    Check MDR Plugin uninstalled

    # No MDR feature but with MDR subscription in ALC policy (MDR does not get installed)
    Remove File  ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    Simulate Send Policy And Run Update  ${MDR_VUT_POLICY}  remove_features=MDR
    Check SSPL Installed
    Check MDR Plugin uninstalled

    # MDR feature and MDR subscription in ALC policy (MDR does get installed)
    Remove File  ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    Simulate Send Policy And Run Update  ${MDR_VUT_POLICY}
    Check SSPL Installed
    Check MDR Plugin Installed
    Check MDR Plugin Installed


*** Keywords ***
Test Teardown
    Stop Update Server
    MDR Test Teardown

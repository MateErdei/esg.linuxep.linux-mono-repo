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

*** Variables ***
${MDR_PLUGIN_PATH}  ${SOPHOS_INSTALL}/plugins/mtr/
${MDR_VUT_POLICY}   ${SUPPORT_FILES}/CentralXml/RealWarehousePolicies/GeneratedAlcPolicies/base_and_mtr_VUT.xml
${BASE_VUT_POLICY}   ${SUPPORT_FILES}/CentralXml/RealWarehousePolicies/GeneratedAlcPolicies/base_only_VUT.xml


*** Test Cases ***
Test MDR Installed Based On MDR Feature And Subscription In ALC Policy Simple
    [Tags]  SMOKE  MDR_PLUGIN   MANAGEMENT_AGENT  OSTIA
    Block Connection Between EndPoint And FleetManager
    Setup Update Scheduler Environment
    Check MDR Plugin uninstalled
    Check SSPL Installed

    # MDR feature and MDR subscription in ALC policy (MDR gets installed)
    Simulate Send Policy And Run Update  ${MDR_VUT_POLICY}
    Check SSPL Installed
    Check MDR Plugin Installed

Test MDR Installed Based On MDR Feature And Subscription In ALC Policy With Combinations
    [Tags]   MDR_PLUGIN   MANAGEMENT_AGENT  OSTIA
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

MDR In Subscription And In Feature List But Missing From Warehouse Should Log Error
    [Documentation]  When MDR in subscription and in feature list but missing from warehouse product should log error
    Block Connection Between EndPoint And FleetManager
    Check MDR Plugin uninstalled
    Check SSPL Installed
    Simulate Send Policy And Run Update  ${BASE_VUT_POLICY}    add_features=MDR    add_subscriptions=ServerProtectionLinux-Plugin-MDR
    Check MDR Plugin uninstalled
    Check Report For Missing Package  ServerProtectionLinux-Plugin-MDR

No Base Or MDR Component In Warehouse Should Report Error And Fail Update
    [Documentation]  When no Base or MDR component in warehouse product should report error and fail update
    [Tags]  MDR_PLUGIN   MANAGEMENT_AGENT  EVENT_PLUGIN  AUDIT_PLUGIN  OSTIA
    Require Fresh Install
    Check SSPL Installed
    Block Connection Between EndPoint And FleetManager

    # Warehouse without Base
    Setup For Test With Warehouse Containing Sensors
    Send Policy With Host Redirection And Run Update  remove_subscriptions=SENSORS

    #Verify Update report error related to missing product (No core base available)
    Check Report For Missing Package  ServerProtectionLinux-Base;ServerProtectionLinux-Plugin-MDR

Test MDR Uninstalled Correctly
    [Tags]   MDR_PLUGIN   MANAGEMENT_AGENT  OSTIA
    Block Connection Between EndPoint And FleetManager
    Check MDR Plugin uninstalled
    Check SSPL Installed

    # MDR feature and MDR subscription in ALC policy (MDR gets installed)
    Simulate Send Policy And Run Update  ${MDR_VUT_POLICY}
    Check SSPL Installed
    Check MDR Plugin Installed

    # No MDR feature but with MDR subscription in ALC policy (MDR does not get installed)
    Simulate Send Policy And Run Update  ${MDR_VUT_POLICY}    remove_features=MDR
    Wait Until MDR Uninstalled


*** Keywords ***
Test Teardown
    Stop Update Server
    MDR Test Teardown

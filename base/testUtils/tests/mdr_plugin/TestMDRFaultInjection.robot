*** Settings ***
Documentation    Suite of fault injection tests for MTR
Resource  MDRResources.robot
Resource  ../scheduler_update/SchedulerUpdateResources.robot
Resource  ../GeneralTeardownResource.robot
Resource  ../upgrade_product/UpgradeResources.robot

Test Setup  Setup Update Scheduler Environment
Test Teardown  Test Teardown

Suite Setup  Setup Ostia Warehouse Environment
Suite Teardown  Teardown Ostia Warehouse Environment

Default Tags   FAULTINJECTION   OSTIA   MDR_PLUGIN

*** Variables ***
${MDR_PLUGIN_POLICY_PATH}   ${SOPHOS_INSTALL}/plugins/mtr/var/policy/mtr.xml
${MDR_VUT_POLICY}           ${SUPPORT_FILES}/CentralXml/RealWarehousePolicies/GeneratedAlcPolicies/base_and_mtr_VUT.xml

*** Test Cases ***
Preventing MDR From Creating Config File Generates An Error In The MTR Log
    Block Connection Between EndPoint And FleetManager
    Simulate Send Policy And Run Update  ${MDR_VUT_POLICY}
    Check SSPL Installed
    Check MDR Plugin Installed

    # Remove MTR file and place directory there instead
    Remove File  ${MDR_PLUGIN_POLICY_PATH}
    Create Directory  ${MDR_PLUGIN_POLICY_PATH}

    # Simulate Updating MDR Policy
    Copy File   ${SUPPORT_FILES}/CentralXml/MDR_policy.xml  ${SOPHOS_INSTALL}/tmp
    Move File   ${SOPHOS_INSTALL}/tmp/MDR_policy.xml  ${SOPHOS_INSTALL}/base/mcs/policy/MDR_policy.xml

    Wait Until Keyword Succeeds
    ...  30s
    ...  1s
    ...  Check Log Contains   /opt/sophos-spl/plugins/mtr/log/mtr.log   Policy writing failed with exception: Could not move /opt/sophos-spl/tmp/mtr.xml_tmp to /opt/sophos-spl/plugins/mtr/var/policy/mtr.xml: Is a directory

*** Keywords ***
Check Log Contains
   [Arguments]  ${LogPath}  ${Message}
   ${LOG_CONTENTS} =  Get File   ${LogPath}
   Should Contain   ${LOG_CONTENTS}   ${Message}

Test Teardown
    Stop Update Server
    MDR Test Teardown
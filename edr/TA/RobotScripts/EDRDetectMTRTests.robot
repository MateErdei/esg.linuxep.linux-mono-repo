*** Settings ***
Documentation    Tests to handle how EDR changes the configuration for Osquery when it detects that MTR is meant to be installed.

Library         Process
Library         OperatingSystem
Library         ../Libs/FakeManagement.py

Resource        EDRResources.robot
Resource        ComponentSetup.robot

Suite Setup     Install With Base SDDS  enableAuditConfig=True
Suite Teardown  Uninstall And Revert Setup

Test Setup      No Operation
Test Teardown   EDR And Base Teardown


*** Test Cases ***
EDR By Default Will Configure Audit Option
    Check EDR Plugin Installed With Base
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check Osquery Configured To Collect Audit Data

EDR Does Not Configure Audit If MTR Is Supposed To Be Installed
    Check MTR in ALC Policy Forces Disable Audit Data Collection

EDR configures osquery to collect audit data if MTR is removed
    Check MTR in ALC Policy Forces Disable Audit Data Collection
    ${alc} =  Get File  ${EXAMPLE_DATA_PATH}/ALC_policy_with_mtr_example.xml
#After installation, sends ALC policy containing EDR and MTR. Verify that the configuration for the live query disables audit data acquisition. Sends a new ALC policy without MTR. Verify that EDR will replace the configuration to enable audit collection of data.

*** Keywords ***
Check MTR in ALC Policy Forces Disable Audit Data Collection
    Check EDR Plugin Installed With Base
    ${alc} =  Get File  ${EXAMPLE_DATA_PATH}/ALC_policy_with_mtr_example.xml
    Send Plugin Policy  edr   ALC  ${alc}
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check Osquery Configured To Not Collect Audit Data


Check Osquery Configured To Collect Audit Data
    ${osqueryConf} =   Get File  ${COMPONENT_ROOT_PATH}/etc/osquery.flags
    Should Contain  ${osqueryConf}  --disable_audit=false


Check Osquery Configured To Not Collect Audit Data
    ${osqueryConf} =   Get File  ${COMPONENT_ROOT_PATH}/etc/osquery.flags
    Should Contain  ${osqueryConf}  --disable_audit=true

*** Settings ***
Documentation    Test EDR installer

#Library         Process
#Library         OperatingSystem

Resource        EDRResources.robot
#Resource        ComponentSetup.robot

#Suite Setup     Install With Base SDDS
#Suite Teardown  Uninstall And Revert Setup

Test Setup       Setup Base And Component
#Test Teardown   EDR And Base Teardown
Test Teardown    Uninstall All



*** Test Cases ***
EDR Disables Auditd After Install With Auditd Running Default Behaviour
    # TODO: Check Auditd Enabled and Running
    Install Base For Component Tests
    Install EDR Directly from SDDS
    # TODO: Check Auditd Stopped and Disabled
    EDR Plugin Log Contains  Successfully stopped service: auditd

EDR Disables Auditd After Install With Auditd Running With Disable Flag
    # TODO: Check Auditd Enabled and Running
    Install Base For Component Tests
    Create Install Options File With Content  --disable-auditd
    Install EDR Directly from SDDS
    # TODO: Check Auditd Stopped and Disabled
    EDR Plugin Log Contains  Successfully stopped service: auditd

EDR Does Disable Auditd After Install With Auditd Running With Do Not Disable Flag
    # TODO: Check Auditd Enabled and Running
    Install Base For Component Tests
    Create Install Options File With Content  --do-not-disable-auditd
    Install EDR Directly from SDDS
    # TODO: Check Auditd Enabled and Running
    EDR Plugin Log Contains  Not disabling auditd

EDR Logs A Failure If It Cannot Disable Auditd After
    # TODO: Check Auditd Enabled and Running
    Install Base For Component Tests
    Create Install Options File With Content  --disable-auditd
    # TODO: Break Auditd Service
    Install EDR Directly from SDDS
    # TODO: Check Auditd Stopped and Disabled
    EDR Plugin Log Contains  Failed to stop service: auditd


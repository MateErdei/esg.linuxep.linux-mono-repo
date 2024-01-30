*** Settings ***
Documentation    Integration tests if SSPL-AV diagnose
Force Tags      INTEGRATION  DIAGNOSE  TAP_PARALLEL2
Library         OperatingSystem
Library         Process
Library         String
Library         XML
Library         ../Libs/fixtures/AVPlugin.py
Library         ${COMMON_TEST_LIBS}/LogUtils.py
Library         ${COMMON_TEST_LIBS}/OnFail.py
Library         ../Libs/ThreatReportUtils.py

Resource        ../shared/AVResources.robot
Resource        ../shared/AVAndBaseResources.robot
Resource        ../shared/DiagnoseResources.robot
Resource        ../shared/ErrorMarkers.robot

Suite Setup     Install With Base SDDS
Suite Teardown   Uninstall All

Test Setup      Diagnose Test Setup
Test Teardown   Diagnose Test TearDown

*** Keywords ***

Diagnose Test Setup
    AV And Base Setup

Diagnose Test TearDown
    Exclude CustomerID Failed To Read Error
    Exclude MCS Router is dead
    Exclude Failed To Update Because JWToken Was Empty
    Exclude UpdateScheduler Fails
    Run Teardown Functions
    Check All Product Logs Do Not Contain Error
    AV And Base Teardown

*** Test Cases ***

Diagnose collects the correct files
    Configure and run scan now
    Run Diagnose And Get DiagnoseOutput
    Check Diagnose Tar Created
    Check Diagnose Collects Correct AV Files
    Check Diagnose Logs

Diagnose collects the onaccessunhealthy file
    ${ON_ACCESS_UNHEALTHY_FILE} =    Set Variable   ${AV_VAR_DIR}/onaccess_unhealthy_flag
    LOG    ${ON_ACCESS_UNHEALTHY_FILE}
    #It is not reasonably possible to emulate the On-Access bad health, which is caused by fanotify failing
    #So we create the file and make sure that it is in the diagnose
    Create File    ${ON_ACCESS_UNHEALTHY_FILE}

    Run Diagnose And Get DiagnoseOutput
    Check Diagnose Tar Created
    Check Diagnose Contains File In AV Var    onaccess_unhealthy_flag
    Check Diagnose Logs

Diagnose collects the threatdetectorunhealthy file
    Register Cleanup  Exclude Susi Initialisation Failed Messages On Access Enabled
    Create SUSI Initialisation Error

    ${rc}   ${output} =    Run And Return Rc And Output   ${AVSCANNER} ${AVSCANNER}
    Log   ${output}
    Should Be Equal As Integers  ${rc}  ${ERROR_RESULT}

    Run Diagnose And Get DiagnoseOutput
    Check Diagnose Tar Created
    Check Diagnose Contains File In TD Var    threatdetector_unhealthy_flag
    Check Diagnose Logs

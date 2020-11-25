*** Settings ***
Documentation    Integration tests of VQA
Force Tags       INTEGRATION  VQA

Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVResources.robot

Library         Collections
Library         Process
Library         ../Libs/LogUtils.py
Library         ../Libs/OnFail.py

Suite Setup     VQA Suite Setup
Suite Teardown  VQA Suite TearDown

Test Setup      VQA Test Setup
Test Teardown   VQA Test TearDown

*** Test Cases ***

SUSI config can scan android file
    Add IDE To Installation  axml.ide
    Check Threat Detected  AndroidManifest.xml  Test/Axml


SUSI config can scan msoffice file
    Add IDE To Installation  office.ide
    Check Threat Detected  cleanmacro.xlsm  Test/Office


SUSI config can scan adobe file
    Add IDE To Installation  pdf.ide
    Check Threat Detected  test.pdf  Pass/URI


SUSI config can scan internet file
    Add IDE To Installation  internet.ide
    Check Threat Detected  test.html  Test/Html


SUSI config can scan zip file as web archive
    Create File  ${SCAN_DIRECTORY}/eicar    ${EICAR_STRING}
    Create Zip   ${SCAN_DIRECTORY}   eicar   eicar.zip

    ${rc}   ${output} =    Run And Return Rc And Output    ${AVSCANNER} ${SCAN_DIRECTORY}/eicar.zip --scan-archives

    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Should Contain  ${output}  Detected "${SCAN_DIRECTORY}/eicar.zip/eicar" is infected with EICAR-AV-Test


*** Variables ***


*** Keywords ***
VQA Suite Setup
    Register On Fail  Debug install set
    Register On Fail  dump log  ${THREAT_DETECTOR_LOG_PATH}
    Register On Fail  dump log  ${AV_LOG_PATH}
    Install With Base SDDS

VQA Suite TearDown
    Log  VQA Suite TearDown

VQA Test Setup
    Check AV Plugin Installed With Base
    Mark AV Log

VQA Test TearDown
    Run Teardown Functions
    #TODO: Remove this line once CORE-2095 is fixed
    #Currently loading more than 1 IDE in test, stops SUSI
    Install With Base SDDS

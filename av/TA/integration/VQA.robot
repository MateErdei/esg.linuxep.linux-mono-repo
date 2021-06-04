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
    Install IDE  axml.ide
    Check Threat Detected  AndroidManifest.xml  Test/Axml


SUSI config can scan msoffice file
    Install IDE  office.ide
    Check Threat Detected  cleanmacro.xlsm  Test/Office


SUSI config can scan adobe file
    Install IDE  pdf.ide
    Check Threat Detected  test.pdf  Pass/URI


SUSI config can scan internet file
    Install IDE  internet.ide
    Check Threat Detected  test.html  Test/Html


SUSI config can scan macintosh file
    Install IDE  hfs.ide
    Check Threat Detected  testfile.hfs  Test/Hfs  /:Disk Image:.journal


SUSI config can scan zip file as web archive
    Create File  ${SCAN_DIRECTORY}/eicar    ${EICAR_STRING}
    Create Zip   ${SCAN_DIRECTORY}   eicar   eicar.zip

    ${rc}   ${output} =    Run And Return Rc And Output    ${AVSCANNER} ${SCAN_DIRECTORY}/eicar.zip --scan-archives

    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Should Contain  ${output}  Detected "${SCAN_DIRECTORY}/eicar.zip/eicar" is infected with EICAR-AV-Test


SUSI config can scan a media file
    Mark Susi Debug Log
    Check File Clean  test.gif
    SUSI Debug Log Contains With Offset  Classifn: 0x93  #TFT Classification ID for grpClean/media


SUSI config can scan a selfextractor file
    Mark Susi Debug Log
    Check File Clean  Firefox.exe
    SUSI Debug Log Contains With Offset  Classifn: 0x58  #TFT Classification ID for grpselfExtractor/selfextractor


*** Variables ***


*** Keywords ***
VQA Suite Setup
    Remove Files
    ...   ${IDE_DIR}/axml.ide
    ...   ${IDE_DIR}/office.ide
    ...   ${IDE_DIR}/pdf.ide
    ...   ${IDE_DIR}/internet.ide
    ...   ${IDE_DIR}/hfs.ide
    Install With Base SDDS
    Check Plugin Installed and Running
    Force SUSI to be initialized

VQA Suite TearDown
    No Operation

VQA Test Setup
    Register On Fail  Debug install set
    Register On Fail  dump log  ${THREAT_DETECTOR_LOG_PATH}
    Register On Fail  dump log  ${AV_LOG_PATH}
    Check Plugin Installed and Running
    Mark AV Log

VQA Test TearDown
    Run Teardown Functions
    Run Keyword If Test Failed   VQA Suite Setup
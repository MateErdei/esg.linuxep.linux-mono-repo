*** Settings ***
Documentation    Integration tests of VQA
Force Tags       INTEGRATION  VQA

Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVResources.robot
Resource    ../shared/ErrorMarkers.robot

Library         Collections
Library         Process
Library         ../Libs/CoreDumps.py
Library         ../Libs/LogUtils.py
Library         ../Libs/OnFail.py

Suite Setup     VQA Suite Setup
Suite Teardown  VQA Suite TearDown

Test Setup      VQA Test Setup
Test Teardown   VQA Test TearDown

*** Test Cases ***

SUSI config can scan android file
    Check Threat Detected  AndroidManifest.xml  Test/SSPL-AXML


SUSI config can scan msoffice file
    Check Threat Detected  cleanmacro.xlsm  Test/SSPL-OFFI


SUSI config can scan adobe file
    Check Threat Detected  test.pdf  Test/SSPL-PDF


SUSI config can scan internet file
    Check Threat Detected  test.html  Test/SSPL-HTML


SUSI config can scan macintosh file
    Check Threat Detected  testfile.hfs  Test/SSPL-HFS  /:Disk Image:file01-1038K.txt


SUSI config can scan zip file as web archive
    Create File  ${SCAN_DIRECTORY}/eicar    ${EICAR_STRING}
    Create Zip   ${SCAN_DIRECTORY}   eicar   eicar.zip

    ${rc}   ${output} =    Run And Return Rc And Output    ${AVSCANNER} ${SCAN_DIRECTORY}/eicar.zip --scan-archives

    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Should Contain  ${output}  Detected "${SCAN_DIRECTORY}/eicar.zip/eicar" is infected with EICAR-AV-Test


SUSI config can scan a media file
    #  "tftClassifications" :
    #  [
    #    {
    #      "description" : "Graphic interchange format",
    #      "group" : "Image",
    #      "name" : "TFT/GIF-A",
    #      "threatLevel" : 1,
    #      "typeId" : "GIF"
    #    }
    #  ]
    Mark Susi Debug Log
    Check File Clean  test.gif
    SUSI Debug Log Contains With Offset  "group" : "Image"
    SUSI Debug Log Contains With Offset  "name" : "TFT/GIF-A"


SUSI config can scan a selfextractor file
    #  "tftClassifications" :
    #  [
    #    {
    #      "description" : "Windows executable",
    #      "group" : "Executable",
    #      "name" : "TFT/EXE-A",
    #      "threatLevel" : 3,
    #      "typeId" : "Windows"
    #    }
    #  ]
    Mark Susi Debug Log
    Check File Clean  Firefox.exe
    SUSI Debug Log Contains With Offset  "group" : "Executable"
    SUSI Debug Log Contains With Offset  "name" : "TFT/EXE-A"


*** Variables ***


*** Keywords ***
VQA Suite Setup
    Install With Base SDDS
    Check Plugin Installed and Running
    Force SUSI to be initialized
    Replace Virus Data With Test Dataset A
    Run IDE update with SUSI loaded

VQA Suite TearDown
    Revert Virus Data To Live Dataset A
    Run IDE update with SUSI loaded

VQA Test Setup
    Register On Fail  Debug install set
    Register On Fail  dump log  ${THREAT_DETECTOR_LOG_PATH}
    Register On Fail  dump log  ${SUSI_DEBUG_LOG_PATH}
    Register On Fail  dump log  ${AV_LOG_PATH}
    Check Plugin Installed and Running
    Mark AV Log
    Register Cleanup   Check All Product Logs Do Not Contain Error
    Register Cleanup   Exclude MCS Router is dead
    Register Cleanup   Exclude CustomerID Failed To Read Error
    Register Cleanup   Require No Unhandled Exception
    Register Cleanup   Check For Coredumps  ${TEST NAME}
    Register Cleanup   Check Dmesg For Segfaults

VQA Test TearDown
    Run Teardown Functions
    Run Keyword If Test Failed   VQA Suite Setup
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
     Register on fail  Debug install set
     Register cleanup  dump log  ${THREAT_DETECTOR_LOG_PATH}
     Register cleanup  dump log  ${AV_LOG_PATH}

     Add IDE to install set  ${IDE_ANDROID_NAME}
     Run installer from install set
     Check IDE present in installation  ${IDE_ANDROID_NAME}

     Copy File   ${RESOURCES_PATH}/file_samples/AndroidManifest.xml  ${SCAN_DIRECTORY}
     ${rc}   ${output} =    Run And Return Rc And Output   ${AVSCANNER} ${SCAN_DIRECTORY}/AndroidManifest.xml
     Log To Console  ${output}
     Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
     Should Contain   ${output}    Detected "${SCAN_DIRECTORY}/AndroidManifest.xml" is infected with Test/Axml


SUSI config can scan zip file as web archive
    Create File  ${SCAN_DIRECTORY}/eicar    ${EICAR_STRING}
    Create Zip   ${SCAN_DIRECTORY}   eicar   eicar.zip

    Run Process     zip  ${SCAN_DIRECTORY}/test.zip  ${SCAN_DIRECTORY}/eicar

    ${rc}   ${output} =    Run And Return Rc And Output    ${AVSCANNER} ${SCAN_DIRECTORY}/eicar.zip --scan-archives

    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Should Contain  ${output}  Detected "${SCAN_DIRECTORY}/eicar.zip/eicar" is infected with EICAR-AV-Test


*** Variables ***
${IDE_ANDROID_NAME}  axml.ide

*** Keywords ***
VQA Suite Setup
    Install With Base SDDS

VQA Suite TearDown
    Log  VQA Suite TearDown

VQA Test Setup
    Check AV Plugin Installed With Base
    Mark AV Log

VQA Test TearDown
    Run Teardown Functions

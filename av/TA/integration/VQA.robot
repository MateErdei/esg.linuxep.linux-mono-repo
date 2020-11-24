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
     Register On Fail  Debug install set
     Register Cleanup  dump log  ${THREAT_DETECTOR_LOG_PATH}
     Register Cleanup  dump log  ${AV_LOG_PATH}

     Add IDE To Install Set  ${IDE_ANDROID_NAME}
     Run Installer From Install Set
     Check IDE Present In Installation  ${IDE_ANDROID_NAME}

     Copy File   ${RESOURCES_PATH}/file_samples/AndroidManifest.xml  ${SCAN_DIRECTORY}
     ${rc}   ${output} =    Run And Return Rc And Output   ${AVSCANNER} ${SCAN_DIRECTORY}/AndroidManifest.xml
     Log To Console  ${output}
     Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
     Should Contain   ${output}    Detected "${SCAN_DIRECTORY}/AndroidManifest.xml" is infected with Test/Axml

     #TODO: Remove this line once CORE-2095 is fixed
     #Currently loading more than 1 IDE in test, stops SUSI
     Uninstall All


SUSI config can scan msoffice file
     Register On Fail  Debug install set
     Register Cleanup  dump log  ${THREAT_DETECTOR_LOG_PATH}
     Register Cleanup  dump log  ${AV_LOG_PATH}

     Add IDE To Install Set  ${IDE_OFFICE_NAME}
     Run Installer From Install Set
     Check IDE Present In Installation  ${IDE_OFFICE_NAME}

     Copy File   ${RESOURCES_PATH}/file_samples/cleanmacro.xlsm  ${SCAN_DIRECTORY}
     ${rc}   ${output} =    Run And Return Rc And Output   ${AVSCANNER} ${SCAN_DIRECTORY}/cleanmacro.xlsm
     Log To Console  ${output}
     Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
     Should Contain   ${output}    Detected "${SCAN_DIRECTORY}/cleanmacro.xlsm" is infected with Test/Office

     #TODO: Remove this line once CORE-2095 is fixed
     #Currently loading more than 1 IDE in test, stops SUSI
     Uninstall All


SUSI config can scan zip file as web archive
    Create File  ${SCAN_DIRECTORY}/eicar    ${EICAR_STRING}
    Create Zip   ${SCAN_DIRECTORY}   eicar   eicar.zip

    ${rc}   ${output} =    Run And Return Rc And Output    ${AVSCANNER} ${SCAN_DIRECTORY}/eicar.zip --scan-archives

    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Should Contain  ${output}  Detected "${SCAN_DIRECTORY}/eicar.zip/eicar" is infected with EICAR-AV-Test

    #TODO: Remove this line once CORE-2095 is fixed
    #Currently loading more than 1 IDE in test, stops SUSI
    Uninstall All


*** Variables ***
${IDE_ANDROID_NAME}  axml.ide
${IDE_OFFICE_NAME}   office.ide

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

*** Settings ***
Documentation    Integration tests of Installer
Force Tags       INTEGRATION  INSTALLER

Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVResources.robot

Library         Collections
Library         Process
Library         ../Libs/LogUtils.py
Library         ../Libs/OnFail.py

Suite Setup     Installer Suite Setup
Suite Teardown  Installer Suite TearDown

Test Setup      Installer Test Setup
Test Teardown   Installer Test TearDown

*** Test Cases ***
SUSI Config Can Scan Android File
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

*** Variables ***
${IDE_ANDROID_NAME}  axml.ide
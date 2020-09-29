*** Settings ***
Documentation    Product tests of sophos_threat_detector
Default Tags    PRODUCT  SOPHOS_THREAT_DETECTOR

Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVResources.robot

Suite Setup     AVSophosThreatDetector Suite Setup
Suite Teardown  AVSophosThreatDetector Suite TearDown

Test Setup      AVSophosThreatDetector Test Setup
Test Teardown   AVSophosThreatDetector Test TearDown

*** Variables ***
${CLI_SCANNER_PATH}  ${COMPONENT_ROOT_PATH}/bin/avscanner

*** Test Cases ***
Test Global Rep works in chroot
    set sophos_threat_detector log level
    Restart sophos_threat_detector
    scan GR test file
    check sophos_threat_dector log for successful global rep lookup

*** Keywords ***

set sophos_threat_detector log level
    Set Log Level  DEBUG

Restart sophos_threat_detector
    Kill sophos_threat_detector
    wait for sophos_threat_detector to be running

wait for sophos_threat_detector to be running
    Wait until threat detector running

Kill sophos_threat_detector
   ${rc}   ${output} =    Run And Return Rc And Output    pgrep sophos_threat
   Run Process   /bin/kill   -9   ${output}

scan GR test file
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${RESOURCES_PATH}/file_samples/gui.exe

check sophos_threat_dector log for successful global rep lookup
    Threat Detector Log Contains  =GR= Connection #0 to host 4.sophosxl.net left intact

AVSophosThreatDetector Suite Setup
    Log  AVSophosThreatDetector Suite Setup
    Install With Base SDDS

AVSophosThreatDetector Suite TearDown
    Log  AVSophosThreatDetector Suite TearDown

AVSophosThreatDetector Test Setup
    Log  AVSophosThreatDetector Test Setup
    Check AV Plugin Installed With Base
    Mark AV Log

AVSophosThreatDetector Test TearDown
    Log  AVSophosThreatDetector Test TearDown

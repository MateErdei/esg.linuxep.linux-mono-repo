*** Settings ***
Documentation    Product tests of sophos_threat_detector
Force Tags       INTEGRATION  SOPHOS_THREAT_DETECTOR

Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVResources.robot

Library         ../Libs/OnFail.py

Suite Setup     AVSophosThreatDetector Suite Setup
Suite Teardown  AVSophosThreatDetector Suite TearDown

Test Setup      AVSophosThreatDetector Test Setup
Test Teardown   AVSophosThreatDetector Test TearDown

*** Variables ***
${CLI_SCANNER_PATH}  ${COMPONENT_ROOT_PATH}/bin/avscanner

*** Test Cases ***
Test Global Rep works in chroot
    run on failure  Run Keyword And Ignore Error  Log File   ${THREAT_DETECTOR_LOG_PATH}  encoding_errors=replace
    set sophos_threat_detector log level
    Restart sophos_threat_detector
    scan GR test file
    check sophos_threat_dector log for successful global rep lookup

Sophos Threat Detector Has No Unnecessary Capabilities
    ${rc}   ${pid} =       Run And Return Rc And Output    pgrep sophos_threat
    ${rc}   ${output} =    Run And Return Rc And Output    getpcaps ${pid}
    Should Not Contain  ${output}  cap_sys_chroot
    # Handle different format of the output from getpcaps on Ubuntu 20.04
    Run Keyword Unless  "${output}" == "Capabilities for `${pid}\': =" or "${output}" == "${pid}: ="  FAIL  msg=Enexpected capabilities: ${output}

*** Keywords ***

set sophos_threat_detector log level
    Set Log Level  DEBUG

Restart sophos_threat_detector
    Kill sophos_threat_detector
    Mark AV Log
    wait for sophos_threat_detector to be running

wait for sophos_threat_detector to be running
    Wait until threat detector running

Kill sophos_threat_detector
   ${rc}   ${output} =    Run And Return Rc And Output    pgrep sophos_threat
   Run Process   /bin/kill   -9   ${output}

scan GR test file
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${RESOURCES_PATH}/file_samples/gui.exe
    run keyword if  ${rc} != ${0}  Log  ${output}
    BuiltIn.Should Be Equal As Integers  ${rc}  ${0}  Failed to scan gui.exe

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
    Run Keyword If Test Failed   Run Keyword And Ignore Error  Log File   ${THREAT_DETECTOR_LOG_PATH}  encoding_errors=replace
    run teardown functions

*** Settings ***
Library   ${COMMON_TEST_LIBS}/LogUtils.py


*** Keywords ***

Exclude RTD fallback error messages
    [Arguments]    ${installDir}=${SOPHOS_INSTALL}
    [Documentation]  TODO: LINUXDAR-8610 Remove these exclusions
    ${rtd_log} =  Set Variable  ${installDir}/plugins/runtimedetections/log/runtimedetections.log
    mark_expected_error_in_log  ${rtd_log}  runtimedetections <> fallback FUSE Loaded provided but not found
    mark_expected_error_in_log  ${rtd_log}  runtimedetections <> fallback USB Module Loaded provided but not found

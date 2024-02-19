*** Settings ***
Resource        ../shared/AVAndBaseResources.robot
Resource        ../shared/AVResources.robot
Suite Setup     Safestore extract suite setup
Suite Teardown   Uninstall All
Force Tags      PRODUCT  SAFESTORE  TAP_PARALLEL1

Test Teardown    Safestore extract Test TearDown

*** Keywords ***
Safestore extract suite setup
    Install With Base SDDS
    AV And Base Setup

    ${av_mark} =  Mark AV Log
    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Check avscanner can detect eicar
    Wait For Log Contains From Mark  ${safestore_mark}  Quarantined ${SCAN_DIRECTORY}/eicar.com successfully


Safestore extract Test TearDown
    Exclude CustomerID Failed To Read Error
    Exclude Failed To Update Because JWToken Was Empty
    Exclude UpdateScheduler Fails
    Exclude MCS Router is dead
    Run Teardown Functions
    Check All Product Logs Do Not Contain Error
    AV And Base Teardown

Run extract tool with threatpath and sha
    [Arguments]    ${path}  ${sha}  ${exit_code}=${0}
    ${rc}   ${output} =    Run And Return Rc And Output    ${COMPONENT_ROOT_PATH}/sbin/safestore_extract_tool --password stuff --destination /tmp/threat.zip --threatpath ${path} --sha256 ${sha}

    Should Be Equal As Integers  ${rc}  ${exit_code}
    [Return]    ${output}

Run extract tool with threatpath
    [Arguments]    ${path}  ${exit_code}=${0}
    ${rc}   ${output} =    Run And Return Rc And Output    ${COMPONENT_ROOT_PATH}/sbin/safestore_extract_tool --password stuff --destination /tmp/threat.zip --threatpath ${path}

    Should Be Equal As Integers  ${rc}  ${exit_code}
    [Return]    ${output}
*** Variables ***
${SUCCESS}    ${0}
${USER_ERROR}    ${1}
${INTERNAL_ERROR}    ${3}
*** Test Cases ***
Test safestore extract tool success both sha and filename
    register cleanup    Remove File    /tmp/threat.zip

    ${output} =    Run extract tool with threatpath and sha  eicar.com  275a021bbfb6489e54d471899f7db9d1663fc695ec2fe2a2c4538aabf651fd0f

    Should Be Equal As Strings  ${output}  {"file":"eicar.com"}
    File Should exist  /tmp/threat.zip

Test safestore extract tool success with sha
    register cleanup    Remove File    /tmp/threat.zip

    ${rc}   ${output} =    Run And Return Rc And Output    ${COMPONENT_ROOT_PATH}/sbin/safestore_extract_tool --password stuff --destination /tmp/threat.zip --sha256 275a021bbfb6489e54d471899f7db9d1663fc695ec2fe2a2c4538aabf651fd0f

    Should Be Equal As Integers  ${rc}  ${SUCCESS}
    Should Be Equal As Strings  ${output}  {"file":"eicar.com"}
    File Should exist  /tmp/threat.zip

Test safestore extract tool success with just filename
    register cleanup    Remove File    /tmp/threat.zip

    ${output} =    Run extract tool with threatpath  eicar.com

    Should Be Equal As Strings  ${output}  {"file":"eicar.com"}
    File Should exist  /tmp/threat.zip

Test safestore extract tool success both sha and directory
    register cleanup    Remove File    /tmp/threat.zip

    ${output} =    Run extract tool with threatpath and sha  ${SCAN_DIRECTORY}/  275a021bbfb6489e54d471899f7db9d1663fc695ec2fe2a2c4538aabf651fd0f
    Should Be Equal As Strings  ${output}  {"file":"eicar.com"}
    File Should exist  /tmp/threat.zip


Test safestore extract tool success with just directory
    register cleanup    Remove File    /tmp/threat.zip

    ${output} =    Run extract tool with threatpath  ${SCAN_DIRECTORY}/
    Should Be Equal As Strings  ${output}  {"file":"eicar.com"}
    File Should exist  /tmp/threat.zip

Test safestore extract tool success both sha and absolute threatpath
    register cleanup    Remove File    /tmp/threat.zip

    ${output} =    Run extract tool with threatpath and sha  ${SCAN_DIRECTORY}/eicar.com  275a021bbfb6489e54d471899f7db9d1663fc695ec2fe2a2c4538aabf651fd0f
    Should Be Equal As Strings  ${output}  {"file":"eicar.com"}
    File Should exist  /tmp/threat.zip


Test safestore extract tool success with just absolute threatpath
    register cleanup    Remove File    /tmp/threat.zip

    ${output} =    Run extract tool with threatpath    ${SCAN_DIRECTORY}/eicar.com
    Should Be Equal As Strings  ${output}  {"file":"eicar.com"}
    File Should exist  /tmp/threat.zip

Test safestore extract tool success with just very long threatpath
    register cleanup    Remove File    /tmp/threat.zip
    ${safeStoreMark} =  Mark Log Size  ${SAFESTORE_LOG_PATH}

    ${s}=     Produce long string
    ${long_filepath}=  Set Variable   /tmp/${s}/${s}/${s}/${s}/${s}/${s}/${s}/${s}/${s}/${s}/${s}/${s}/${s}/${s}/${s}
    Create File  ${long_filepath}    ${EICAR_STRING}
    Register Cleanup   Remove Directory  /tmp/${s}  recursive=True

    Check avscanner can detect eicar in  ${long_filepath}
    Wait For Log Contains From Mark    ${safestore_mark}     Quarantined ${long_filepath} successfully

    ${output} =    Run extract tool with threatpath     ${long_filepath}

    Should Be Equal As Strings  ${output}  {"file":"${s}"}
    File Should exist  /tmp/threat.zip

Test safestore extract tool fails with very long destPath

    ${s}=     Produce long string
    ${long_filepath}=  Set Variable   /tmp/${s}/${s}/${s}/${s}/${s}/${s}/${s}/${s}/${s}/${s}/${s}/${s}/${s}/${s}/thing.zip
    register cleanup    Remove File    ${long_filepath}
    ${rc}   ${output} =    Run And Return Rc And Output    ${COMPONENT_ROOT_PATH}/sbin/safestore_extract_tool --password stuff --destination ${long_filepath} --threatpath ${SCAN_DIRECTORY}/eicar.com

    Should Be Equal As Integers  ${rc}  ${INTERNAL_ERROR}
    Should Be Equal As Strings  ${output}   	{"errorMsg":"Failed to compress threat","file":"eicar.com"}
    File Should not exist  ${long_filepath}
    mark_expected_error_in_log  ${AV_PLUGIN_PATH}/log/safestore_extract.log  Error opening zip file: /tmp/
    mark_expected_error_in_log  ${AV_PLUGIN_PATH}/log/safestore_extract.log  Failed to compress threat with error code

Test safestore returns failure if no threat found
    ${output} =    Run extract tool with threatpath     noteicar    ${USER_ERROR}

    Should Be Equal As Strings  ${output}  {"errorMsg":"Failed to find matching threat in safestore database","file":"noteicar"}
    mark_expected_error_in_log  ${AV_PLUGIN_PATH}/log/safestore_extract.log  Failed to find matching threat in safestore database
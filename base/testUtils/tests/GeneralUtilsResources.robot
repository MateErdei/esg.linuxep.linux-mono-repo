*** Settings ***
Library     Process

*** Keywords ***
Run Shell Process
    [Arguments]  ${Command}   ${OnError}   ${timeout}=20s
    ${result} =   Run Process  ${Command}   shell=True   timeout=${timeout}
    Should Be Equal As Integers  ${result.rc}  ${0}   ${OnError}.\n${SPACE}stdout: \n${result.stdout} \n${SPACE}stderr: \n${result.stderr}
    [Return]  ${result}


File Should Contain
    [Arguments]  ${file_path}  ${expected_contents}
    ${contents}=  Get File   ${file_path}
    Should Contain  ${contents}   ${expected_contents}

File Should Not Contain
    [Arguments]  ${file_path}  ${expected_contents}
    ${contents}=  Get File   ${file_path}
    Should Not Contain  ${contents}   ${expected_contents}
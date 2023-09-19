*** Settings ***
Library         Process

*** Keywords ***
Run Shell Process
    [Arguments]  ${Command}   ${OnError}   ${timeout}=20s  ${stdout}=/tmp/shell_process_stdout
    ${result} =   Run Process  ${Command}   shell=True   timeout=${timeout}  stdout=${stdout}
    Should Be Equal As Integers  ${result.rc}  ${0}   ${OnError}.\n${SPACE}stdout: \n${result.stdout} \n${SPACE}stderr: \n${result.stderr}
    [Return]  ${result}

*** Settings ***
Library    OperatingSystem
Library    Process

Library    ${LIB_FILES}/OnFail.py

*** Keywords ***
Run Shell Process
    [Arguments]  ${command}   ${onError}   ${timeout}=20s
    ${result} =   Run Process  ${command}   shell=True   timeout=${timeout}
    Should Be Equal As Integers  ${result.rc}  ${0}   ${onError}.\n${SPACE}stdout: \n${result.stdout} \n${SPACE}stderr: \n${result.stderr}
    [Return]  ${result}

Create Temporary File
    [Arguments]    ${filePath}    ${fileContents}
    Create File    ${filePath}    ${fileContents}
    register_cleanup    Remove File    ${filePath}

File Should Contain
    [Arguments]  ${filePath}  ${expectedContents}
    ${contents}=  Get File   ${filePath}
    Should Contain  ${contents}   ${expectedContents}

File Should Not Contain
    [Arguments]  ${filePath}  ${expectedContents}
    ${contents}=  Get File   ${filePath}
    Should Not Contain  ${contents}   ${expectedContents}

Verify Permissions
    [Arguments]    ${path}    ${mode}   ${user}=${EMPTY}   ${group}=${EMPTY}
    ${stat_result} =    Evaluate   os.stat("${path}")

    ${actual_mode} =   Evaluate   oct(stat.S_IMODE(${stat_result.st_mode}))
    Should Be Equal    ${actual_mode}    ${mode}     msg="incorrect mode for ${path}"

    IF   '${user}' != '${EMPTY}'
        ${actual_user} =   Evaluate   pwd.getpwuid(${stat_result.st_uid}).pw_name
        Should Be Equal   ${actual_user}   ${user}     msg="incorrect user for ${path}"
    END

    IF   '${group}' != '${EMPTY}'
        ${actual_group} =   Evaluate   grp.getgrgid(${stat_result.st_gid}).gr_name
        Should Be Equal   ${actual_group}   ${group}     msg="incorrect group for ${path}"
    END
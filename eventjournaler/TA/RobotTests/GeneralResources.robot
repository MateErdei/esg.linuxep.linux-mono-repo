*** Settings ***
Library         Process
Library         OperatingSystem
*** Keywords ***
File Should Contain
    [Arguments]  ${file}  ${string_to_contain}
    ${content} =  Get File  ${file}
    Should Contain  ${content}   ${string_to_contain}

File Should Contain Only
    [Arguments]  ${file}  ${string}
    ${content} =  Get File  ${file}
    Should Be Equal As Strings  ${content}   ${string}

File Should Not Contain Only
    [Arguments]  ${file}  ${string}
    ${content} =  Get File  ${file}
    Should Not Be Equal As Strings  ${content}   ${string}

Mark File
    [Arguments]  ${path}
    ${content} =  Get File   ${path}
    Log  ${content}
    ${mark} =  Evaluate  ${content.split("\n").__len__()} - 1
    [Return]  ${mark}

Marked File Contains
    [Arguments]  ${path}  ${input}  ${mark}
    ${content} =  Get File   ${path}
    ${content} =  Evaluate  "\\n".join(${content.__repr__()}.split("\\n")[${mark}:])
    Should Contain  ${content}  ${input}

Marked File Contains X Times
    [Arguments]  ${path}  ${input}  ${xtimes}  ${mark}
    ${content} =  Get File   ${path}
    ${content} =  Evaluate  "\\n".join(${content.__repr__()}.split("\\n")[${mark}:])
    Should Contain X Times  ${content}  ${input}  ${xtimes}

Marked File Does Not Contain
    [Arguments]  ${path}  ${input}  ${mark}
    ${content} =  Get File   ${path}
    ${content} =  Evaluate  "\\n".join(${content.__repr__()}.split("\\n")[${mark}:])
    Should Not Contain  ${content}  ${input}

Run Shell Process
    [Arguments]  ${Command}   ${OnError}   ${timeout}=20s
    ${result} =   Run Process  ${Command}   shell=True   timeout=${timeout}
    Should Be Equal As Integers  ${result.rc}  0   "${OnError}.\nstdout: \n${result.stdout} \n. stderr: \n${result.stderr}"

Remove Subscriber Socket
    Run Shell Process  rm -rf ${SOPHOS_INSTALL}/var/ipc/events.ipc   Removing subscriber soocket failed

*** Keywords ***
Wait For EDR to be Installed

    Wait Until Keyword Succeeds
    ...   40 secs
    ...   10 secs
    ...   File Should exist    ${SOPHOS_INSTALL}/plugins/edr/bin/edr


    Wait Until Keyword Succeeds
    ...   10 secs
    ...   2 secs
    ...   EDR Plugin Is Running

EDR Plugin Is Running
    ${result} =    Run Process  pgrep  edr
    Should Be Equal As Integers    ${result.rc}    0

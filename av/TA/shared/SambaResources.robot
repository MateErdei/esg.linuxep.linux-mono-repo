*** Settings ***
Library         Process
Library         OperatingSystem
Library         ../Libs/SambaUtils.py
Library         ${COMMON_TEST_LIBS}/OnFail.py

*** Variables ***
${SAMBA_CONFIG}    /etc/samba/smb.conf

*** Keywords ***

Create Local SMB Share
    [Arguments]  ${source}  ${destination}  ${opts}=
    file should exist  ${SAMBA_CONFIG}
    file should not exist  ${SAMBA_CONFIG}_bkp
    Copy File  ${SAMBA_CONFIG}  ${SAMBA_CONFIG}_bkp
    Register Cleanup  Restore Samba config
    ${share_config}=  catenate   SEPARATOR=
    ...   [testSamba]\n
    ...   comment = SMB Share\n
    ...   path = ${source}\n
    ...   browseable = yes\n
    ...   writable = yes\n
    ...   guest ok = yes\n
    Append To File  ${SAMBA_CONFIG}   ${share_config}

    Allow Samba To Access Share  ${source}

    Reload Samba

    Wait Until Keyword Succeeds
    ...  10x
    ...  100 ms
    ...  Mount Samba Share  ${destination}   ${opts}

    ${rc}   ${output} =    Run And Return Rc And Output    mount
    Log  ${output}
    Register Cleanup  Remove Local SMB Share   ${source}   ${destination}

Restore Samba config
    Move File  ${SAMBA_CONFIG}_bkp  ${SAMBA_CONFIG}
    Reload Samba

Remove Local SMB Share
    [Arguments]  ${source}  ${destination}
    Unmount Test Mount  ${destination}
    Remove Directory    ${destination}  recursive=True
    Remove Directory    ${source}  recursive=True

Restart Samba
    ${result} =  Run Process  which  apt-get
    Run Keyword If  "${result.rc}" == "0"
        ...   Run Shell Process   systemctl restart smbd  OnError=Failed to restart SMB server
        ...   ELSE
        ...   Run Shell Process   systemctl restart smb   OnError=Failed to restart SMB server

Start Samba
    ${result} =  Run Process  which  apt-get
    Run Keyword If  "${result.rc}" == "0"
        ...   Run Shell Process   systemctl start smbd  OnError=Failed to start SMB server
        ...   ELSE
        ...   Run Shell Process   systemctl start smb   OnError=Failed to start SMB server

Reload Samba
    Run Process  smbcontrol  all  reload-config


Mount Samba Share
    [Arguments]  ${destination}   ${opts}=defaults
    ${result} =  Run Process   mount  -t  cifs  //localhost/testSamba  ${destination}  -o  guest,${opts}
    Pass Execution If    'Invalid argument' in $result.stderr
    ...     ${opts} is not a supported CIFS mount option with this installation - skipping test
    Should Be Equal As Integers  ${result.rc}  ${0}  "Failed to mount local SMB share.\n${SPACE}stdout: \n${result.stdout} \n${SPACE}stderr: \n${result.stderr}"
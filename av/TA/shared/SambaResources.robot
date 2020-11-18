*** Settings ***
Library         Process
Library         OperatingSystem

Resource    RunShellProcess.robot

*** Variables ***
${SAMBA_CONFIG}    /etc/samba/smb.conf

*** Keywords ***
Create Local SMB Share
    [Arguments]  ${source}  ${destination}
    file should not exist  ${SAMBA_CONFIG}_bkp
    Copy File  ${SAMBA_CONFIG}  ${SAMBA_CONFIG}_bkp
    ${share_config}=  catenate   SEPARATOR=
    ...   [testSamba]\n
    ...   comment = SMB Share\n
    ...   path = ${source}\n
    ...   browseable = yes\n
    ...   read only = yes\n
    ...   guest ok = yes\n
    Append To File  ${SAMBA_CONFIG}   ${share_config}

    Restart Samba
    Run Shell Process   mount -t cifs \\\\\\\\localhost\\\\testSamba ${destination} -o guest   OnError=Failed to mount local SMB share

Remove Local SMB Share
    [Arguments]  ${source}  ${destination}
    Run Shell Process   umount ${destination}   OnError=Failed to unmount local SMB server
    Remove Directory    ${destination}  recursive=True
    Move File  ${SAMBA_CONFIG}_bkp  ${SAMBA_CONFIG}
    Restart Samba
    Remove Directory    ${source}  recursive=True

Restart Samba
    ${result} =  Run Process  which  yum
    Run Keyword If  "${result.rc}" == "0"
        ...   Run Shell Process   systemctl restart smb  OnError=Failed to restart SMB server
        ...   ELSE
        ...   Run Shell Process   systemctl restart smbd   OnError=Failed to restart SMB server

*** Settings ***
Library         Process
Library         OperatingSystem

Resource    RunShellProcess.robot

*** Variables ***
${SAMBA_CONFIG}    /etc/samba/smb.conf

*** Keywords ***
Allow Samba To Access Share
    [Arguments]  ${source}
    ${result} =   Run Process  semanage  fcontext  -a  -t  samba_share_t  ${source}(/.*)?
    Log  "semanage:${SPACE}stdout: \n${result.stdout} \n${SPACE}stderr: \n${result.stderr}"
    ${result} =   Run Process  restorecon  -Rv  ${source}
    Log  "restorecon:${SPACE}stdout: \n${result.stdout} \n${SPACE}stderr: \n${result.stderr}"
    ${result} =   Run Process  sestatus
    Log  "sestatus:${SPACE}stdout: \n${result.stdout} \n${SPACE}stderr: \n${result.stderr}"

Create Local SMB Share
    [Arguments]  ${source}  ${destination}
    file should not exist  ${SAMBA_CONFIG}_bkp
    Copy File  ${SAMBA_CONFIG}  ${SAMBA_CONFIG}_bkp
    Register Cleanup  Restore Samba config
    ${share_config}=  catenate   SEPARATOR=
    ...   [testSamba]\n
    ...   comment = SMB Share\n
    ...   path = ${source}\n
    ...   browseable = yes\n
    ...   read only = yes\n
    ...   guest ok = yes\n
    Append To File  ${SAMBA_CONFIG}   ${share_config}

    Allow Samba To Access Share  ${source}

    Restart Samba
    Run Shell Process   mount -t cifs \\\\\\\\localhost\\\\testSamba ${destination} -o guest   OnError=Failed to mount local SMB share

Restore Samba config
    Move File  ${SAMBA_CONFIG}_bkp  ${SAMBA_CONFIG}

Remove Local SMB Share
    [Arguments]  ${source}  ${destination}
    Run Shell Process   umount ${destination}   OnError=Failed to unmount local SMB server
    Restart Samba
    Remove Directory    ${destination}  recursive=True
    Remove Directory    ${source}  recursive=True

Restart Samba
    ${result} =  Run Process  which  yum
    Run Keyword If  "${result.rc}" == "0"
        ...   Run Shell Process   systemctl restart smb  OnError=Failed to restart SMB server
        ...   ELSE
        ...   Run Shell Process   systemctl restart smbd   OnError=Failed to restart SMB server

*** Settings ***
Library     Process
Library     Collections
Library     OperatingSystem

Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library     ${LIBS_DIRECTORY}/CapnSubscriber.py
Library     ${LIBS_DIRECTORY}/SshSupport.py
Library     ${LIBS_DIRECTORY}/AuditSupport.py
Library     ${LIBS_DIRECTORY}/UserUtils.py
Library     ${LIBS_DIRECTORY}/OSUtils.py

Resource  ../sul_downloader/SulDownloaderResources.robot
Resource  ../installer/InstallerResources.robot
Resource  AuditPluginResources.robot
Resource  ../GeneralTeardownResource.robot

Test Setup  Run Keywords
...         Ensure a clean start  AND
...         Setup SSPL Audit Plugin Tmpdir  AND
...         Require Fresh Install

Test Teardown     Run Keywords
...               General Test Teardown  AND
...               Default Test Teardown  AND
...               Delete User  temptestuser  True  AND
...               Remove File  /tmp/example.sh  AND
...               Remove File  /usr/bin/sophos-touch  AND
...               Remove Directory   ${tmpdir}    recursive=True


Default Tags  AUDIT_PLUGIN  PUB_SUB  MANAGEMENT_AGENT


*** Test Cases ***

SSPL Check RigidName is correct
    ${SDDS} =  Get SSPL Audit Plugin SDDS
    Check Sdds Import Matches Rigid Name  ${SSPL_AUDIT_RIGID_NAME}  ${SDDS}

SSPL Audit Plugin Direct Installation
    Install Audit Plugin Directly
    # file matches the directory permission but executable flag
    Ensure Chmod File Matches        /etc/audisp/plugins.d/                 750
    Ensure Owner and Group Matches   /etc/audisp/plugins.d/                 root  root
    Ensure Chmod File Matches        /etc/audisp/plugins.d/sspl-audit.conf  640
    Ensure Owner and Group Matches   /etc/audisp/plugins.d/sspl-audit.conf  root  root

    Ensure Chmod File Matches        /etc/audit/rules.d/50_sophos_exec.rules  600
    Ensure Owner and Group Matches   /etc/audit/rules.d/50_sophos_exec.rules  root  root

#FIXME: Requirement for the moment is that auditd must be installed, to be reviewed in LINUXEP-7528
#SSPL Audit Plugin Installation When Machine Does not have Auditd
#    Ensure Auditd Is Not Installed
#    Install Audit Plugin Directly

SSPL Audit Plugin Warehouse Installation
    Install Audit Plugin From Warehouse

SSPL Audit Plugin And Subscriber When Running Failed ssh attempt
    Install Audit Plugin Directly
    Disable System Auditd
    Start Subscriber
    Run AuditPlugin Against Example  failed_ssh_password_attempt
    ${channel} =  Get Credential Channel
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Clear Cache And Check Capn Output For A Single Message  ${channel}  targetUserSID.username=testuser  sessionType=networkInteractive

# amazon linux by default allow only ssh via key, not with password
SSPL Audit Plugin Simulate An Ssh attempt using password
    [Tags]  AUDIT_PLUGIN  EXCLUDE_AWS  PUB_SUB  MANAGEMENT_AGENT
    Install Audit Plugin Directly
    Start Subscriber
    Simulate Failed Ssh Using Password
    ${channel} =  Get Credential Channel
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Clear Cache And Check Capn Output For A Single Message  ${channel}  subjectUserSID.userid=0  sessionType=networkInteractive


SSPL Audit Plugin Simulate An Ssh attempt
    Install Audit Plugin Directly
    Start Subscriber
    Simulate Failed Ssh Using PublicKey
    ${channel} =  Get Credential Channel
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Clear Cache And Check Capn Output For A Single Message  ${channel}  subjectUserSID.userid=0  sessionType=networkInteractive

Create User And Check Capn Output Is Correct
    Install Audit Plugin Directly
    Start Subscriber
    Add User  temptestuser
    ${channel} =  Get Credential Channel
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Clear Cache And Check Capn Output For A Single Message  ${channel}  eventType=created  userGroupName=temptestuser  sessionType=interactive  subjectUserSID.userid=0  targetUserSID.username=temptestuser

Run Process And Check Capn Output Is Correct
    Install Audit Plugin Directly
    Start Subscriber

    ${touchPath} =  Run Process  which  touch
    ${touchPath} =  Set Variable  ${touchPath.stdout}
    Copy File  ${touchPath}  /usr/bin/sophos-touch

    ${result} =  Run Process  sophos-touch   ${tmpdir}/a-temp-file
    ${channel} =  Get Process Channel
    ${message} =  Check Capn Output For A Single Message   ${channel}
    ...  eventType=start
    ...  ownerUserSID.userid=0
    ...  cmdLine=sophos-touch ${tmpdir}/a-temp-file
    ...  pathname.pathname=/usr/bin/sophos-touch
    ...  pathname.flags=0
    ...  pathname.fileSystemType=0
    ...  pathname.driveLetter=0
    ...  pathname.openName.length=21
    ...  pathname.openName.offset=0
    ...  pathname.volumeName.length=0
    ...  pathname.volumeName.offset=0
    ...  pathname.shareName.length=0
    ...  pathname.shareName.offset=0
    ...  pathname.extensionName.length=0
    ...  pathname.extensionName.offset=0
    ...  pathname.streamName.length=0
    ...  pathname.streamName.offset=0
    ...  pathname.finalComponentName.length=12
    ...  pathname.finalComponentName.offset=9
    ...  pathname.parentDirName.length=9
    ...  pathname.parentDirName.offset=0

    ${proctitle} =  Get From Dictionary  ${message}  procTitle
    Should Contain  ${proctitle}  robot
    Should Contain  ${proctitle}  python


Run Process And Check Capn Output Is Correct With Full Path To Executable
    Install Audit Plugin Directly
    Start Subscriber

    ${touchPath} =  Run Process  which  touch
    ${touchPath} =  Set Variable  ${touchPath.stdout}
    Copy File  ${touchPath}  /usr/bin/sophos-touch

    ${result} =  Run Process  /usr/bin/sophos-touch     ${tmpdir}/a-temp-file
    ${channel} =  Get Process Channel
    ${message} =  Check Capn Output For A Single Message   ${channel}
    ...  eventType=start
    ...  ownerUserSID.userid=0
    ...  cmdLine=/usr/bin/sophos-touch ${tmpdir}/a-temp-file
    ...  procTitle=/usr/bin/sophos-touch ${tmpdir}/a-temp-file
    ...  pathname.pathname=/usr/bin/sophos-touch
    ...  pathname.flags=0
    ...  pathname.fileSystemType=0
    ...  pathname.driveLetter=0
    ...  pathname.openName.length=21
    ...  pathname.openName.offset=0
    ...  pathname.volumeName.length=0
    ...  pathname.volumeName.offset=0
    ...  pathname.shareName.length=0
    ...  pathname.shareName.offset=0
    ...  pathname.extensionName.length=0
    ...  pathname.extensionName.offset=0
    ...  pathname.streamName.length=0
    ...  pathname.streamName.offset=0
    ...  pathname.finalComponentName.length=12
    ...  pathname.finalComponentName.offset=9
    ...  pathname.parentDirName.length=9
    ...  pathname.parentDirName.offset=0


Run Process And Check Capn Output Is Correct With Executable With Extension
    Install Audit Plugin Directly
    Start Subscriber
    Copy File  /bin/touch  /tmp/example.sh
    Run Process  /tmp/example.sh  ${tmpdir}/another-temp-file
    ${channel} =  Get Process Channel
    ${message} =  Check Capn Output For A Single Message   ${channel}
    ...  eventType=start
    ...  ownerUserSID.userid=0
    ...  cmdLine=/tmp/example.sh ${tmpdir}/another-temp-file
    ...  procTitle=/tmp/example.sh ${tmpdir}/another-temp-file
    ...  pathname.pathname=/tmp/example.sh
    ...  pathname.flags=0
    ...  pathname.fileSystemType=0
    ...  pathname.driveLetter=0
    ...  pathname.openName.length=15
    ...  pathname.openName.offset=0
    ...  pathname.volumeName.length=0
    ...  pathname.volumeName.offset=0
    ...  pathname.shareName.length=0
    ...  pathname.shareName.offset=0
    ...  pathname.extensionName.length=2
    ...  pathname.extensionName.offset=13
    ...  pathname.streamName.length=0
    ...  pathname.streamName.offset=0
    ...  pathname.finalComponentName.length=10
    ...  pathname.finalComponentName.offset=5
    ...  pathname.parentDirName.length=5
    ...  pathname.parentDirName.offset=0


SSPL Audit Plugin Is Restarted After Kill And Works Normally
    Install Audit Plugin Directly
    Kill Audit Plugin
    # give some time to audit plugin to start up correctly
    Sleep  5 secs

    Start Subscriber
    Simulate Failed Ssh Using Public Key

    ${channel} =  Get Credential Channel
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Clear Cache And Check Capn Output For A Single Message  ${channel}  subjectUserSID.userid=0  sessionType=networkInteractive

*** Settings ***
Library    Process
Library    OperatingSystem

Library     ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library     ${LIBS_DIRECTORY}/LogUtils.py
Resource  ../installer/InstallerResources.robot
Resource  ../watchdog/LogControlResources.robot


*** Keywords ***
Start Management Agent
    Set Log Level For Component And Reset and Return Previous Log  sophos_managementagent  DEBUG
    ${MngtHandle} =   Start Process    ${SOPHOS_INSTALL}/base/bin/sophos_managementagent   shell=yes    stdout=${tmpdir}/management.log  stderr=${tmpdir}/management.log  env:SOPHOS_PUB_SUB_ROUTER_LOGGING=1
    Set Suite Variable    ${GL_MngtHandle}    ${MngtHandle}
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  5 secs
    ...  Check Management Agent Running And Ready

Stop Management Agent
    Terminate Process       ${GL_MngtHandle}

Check Managment Agent Is Not Running
    ${result} =    Run Process  pgrep  sophos_managementagent
    Should Not Be Equal As Integers    ${result.rc}    0   msg="stdout:${result.stdout}\nerr: ${result.stderr}"

Wait For Management Agent To Stop
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Check Managment Agent Is Not Running

Setup Tmpdir
    Set Test Variable    ${tmpdir}     ./tmp
    Remove Directory   ${tmpdir}    recursive=True
    Create Directory   ${tmpdir}

Setup Tmp Dir And Stop Watchdog
    Setup Tmpdir

    Should Exist   ${SOPHOS_INSTALL}/var/ipc
    Should Exist   ${SOPHOS_INSTALL}/logs/base
    Should Exist   ${SOPHOS_INSTALL}/tmp
    Should Exist   ${SOPHOS_INSTALL}/base/mcs/status

    # stop all spl services, test will need to handle start and stop of management agent
    # At the moment the install.sh script will install and start all services.  This may change later.
    Run Process   systemctl stop sophos-spl   shell=yes
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  0.5 secs
    ...  Check Watchdog Not Running


Management Agent Test Setup
    Setup Tmp Dir And Stop Watchdog
    #As we are only testing management agent we can remove all plugins from the registry.
    #This will speed up management agent starting as it won't get stuck waiting for a reply from
    #each plugin when it starts up and tries to register with them.
    Remove File  ${SOPHOS_INSTALL}/base/pluginRegistry/*.json


Check Management Agent Running And Ready
    Check Management Agent Running
    Sleep  0.01  # Give management agent chance top write "Initializing Management Agent" in log
    Check Log Contains More Than One Pair Of Strings In Order  Initializing Management Agent  Management Agent running  ${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log  Management Agent Log


Check File Content
    [Arguments]     ${expectedFileContent}     ${pathToCheck}
    # check if mcs file has been created on disk
    @{files} = 	List Files In Directory	   ${pathToCheck}

    ${result} =  Set Variable   False

    ${FILE_CONTENT} =  Set Variable  ''

    FOR    ${item}     IN      @{files}
    ${filecontent} =     Get File    ${pathToCheck}/${item}
    Log File    ${pathToCheck}/${item}
    ${result} =   Set Variable If   """${filecontent}""" == """${expectedFileContent}"""     True
    Run Keyword If   ${result}   Exit For Loop
    END

   Should Be True   ${result}

Check Event File
    [Arguments]     ${expectedFileContent}
    Check File Content    ${expectedFileContent}    ${SOPHOS_INSTALL}/base/mcs/event

Remove Event Xml Files
    @{eventfiles} = 	List Files In Directory 	${SOPHOS_INSTALL}/base/mcs/event

    FOR    ${item}     IN      @{eventfiles}
    Remove File   ${SOPHOS_INSTALL}/base/mcs/event/${item}
    END

Remove Action Xml Files
    @{actionfiles} = 	List Files In Directory  	${SOPHOS_INSTALL}/base/mcs/action

    FOR    ${item}     IN      @{actionfiles}
    Remove File   ${SOPHOS_INSTALL}/base/mcs/action/${item}
    END

Remove Status Xml Files
    @{statusfiles} = 	List Files In Directory 	${SOPHOS_INSTALL}/base/mcs/status

    FOR    ${item}     IN      @{statusfiles}
    Remove File   ${SOPHOS_INSTALL}/base/mcs/status/${item}
    END

    @{statuscachefiles} = 	List Files In Directory 	${SOPHOS_INSTALL}/base/mcs/status/cache

    FOR    ${item}     IN      @{statuscachefiles}
    Remove File   ${SOPHOS_INSTALL}/base/mcs/status/cache/${item}
    END


Check File With Wait
    [Arguments]  ${expectedFileContent}  ${file}
    Wait Until Keyword Succeeds
    ...  10
    ...  1
    ...  Check File Content    ${expectedFileContent}  ${file}

Check Status File
    [Arguments]     ${expectedFileContent}
    Check File With Wait  ${expectedFileContent}  ${SOPHOS_INSTALL}/base/mcs/status

Check Status Cache File
    [Arguments]     ${expectedFileContent}
    Check File With Wait  ${expectedFileContent}  ${SOPHOS_INSTALL}/base/mcs/status/cache

Require Watchdog Running
    ${systemctlResult} =  Run Process   systemctl start sophos-spl   shell=yes
    Should Be Equal As Integers    ${systemctlResult.rc}    0   ${systemctlResult.stderr}
    Check Watchdog Running
*** Settings ***
Resource  ../sul_downloader/SulDownloaderResources.robot
Resource  ../installer/InstallerResources.robot
Resource  ../watchdog/WatchdogResources.robot


*** Keywords ***
Setup Example Plugin Tmpdir
    Set Test Variable  ${tmpdir}            /tmp/Example-Plugin-Tests
    Set Test Variable  ${examplepluginlog}  ${SOPHOS_INSTALL}/plugins/Example/log/Example.log
    Remove Directory   ${tmpdir}    recursive=True
    Create Directory   ${tmpdir}
    Create Directory   ${tmpdir}/scanpath
    Create File        ${tmpdir}/scanpath/novirus.txt   "any content"
    Run Process  chown  sophos-spl-user:sophos-spl-group  ${tmpdir}
    Run Process  chown  -R  sophos-spl-user:sophos-spl-group  ${tmpdir}

Install Example Plugin Test Setup
    Require Fresh Install
    Setup Example Plugin Tmpdir
    Install Plugin Example

Install Plugin Example
    ${examplepluginsdds}=  Get Example Plugin sdds
    Install Package Via Warehouse  ${examplepluginsdds}    ${EXAMPLE_PLUGIN_RIGID_NAME}

Install Plugin Example From
    [Arguments]  ${examplepluginsdds}
    Install Package Via Warehouse  ${examplepluginsdds}    ${EXAMPLE_PLUGIN_RIGID_NAME}

Check Example Plugin Running
    ${result} =    Run Process  pgrep  example
    Should Be Equal As Integers    ${result.rc}    0

Check Example Plugin Not Running
    ${result} =    Run Process  pgrep  example
    Should Not Be Equal As Integers    ${result.rc}    0

Check Example Plugin Log Contains
    [Arguments]  ${pattern}  ${fail_message}
    Log File  ${examplepluginlog}
    ${ret} =  Grep File  ${examplepluginlog}  ${pattern}
    Should Contain  ${ret}  ${pattern}  ${fail_message}
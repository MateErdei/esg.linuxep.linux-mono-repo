*** Settings ***

Resource  ../management_agent-audit_plugin/AuditPluginResources.robot
Resource  ../management_agent-event_processor/EventProcessorResources.robot

Test Setup  Run Keywords
...         Install Audit Plugin Directly  AND
...         Install EventProcessor Plugin Directly  AND
...         Empty Directory  ${SOPHOS_INSTALL}/base/mcs/event/

Test Teardown   Run Keywords
...             AuditPluginResources.Default Test Teardown  AND
...             Uninstall Audit Plugin  AND
...             Uninstall EventProcessor Plugin  AND
...             Empty Directory  ${SOPHOS_INSTALL}/base/mcs/event/  AND
...             Run Keyword If Test Failed   Require Fresh Install

Default Tags  AUDIT_PLUGIN  EVENT_PLUGIN  PUB_SUB

*** Test Cases ***

Wget Is Reported In Event XML
    ${result} =  Run Process  wget  sophos.com
    ${Description}=  Set Variable  "Unsafe process was started"
    ${Name}=  Set Variable  "ATTENTION: unsafe process was started"
    Wait Until Keyword Succeeds
    ...  3 secs
    ...  1 secs
    ...  SAV Event File Exists And Contains Correct Contents   ${Description}  ${Name}   1


Wget Is The Only Process Reported In Event XML
    Start Subscriber

    # Find paths so that we know what to look for in capn output.
    ${touchPath} =  Run Process  which  touch
    ${touchPath} =  Run Process  readlink  -f  ${touchPath.stdout}
    ${touchPath} =  Set Variable  ${touchPath.stdout}

    ${pingPath} =  Run Process  which  ping
    ${pingPath} =  Run Process  readlink  -f  ${pingPath.stdout}
    ${pingPath} =  Set Variable  ${pingPath.stdout}

    ${rmPath} =  Run Process  which  rm
    ${rmPath} =  Run Process  readlink  -f  ${rmPath.stdout}
    ${rmPath} =  Set Variable  ${rmPath.stdout}

    ${result} =  Run Process  touch  junk
    ${result} =  Run Process  rm  -f  junk
    ${result} =  Run Process  ping  sophos.com  -c  2
    ${result} =  Run Process  wget  sophos.com
    ${Description}=  Set Variable  "Unsafe process was started"
    ${Name}=  Set Variable  "ATTENTION: unsafe process was started"

    Wait Until Keyword Succeeds
    ...  3 secs
    ...  1 secs
    ...  SAV Event File Exists And Contains Correct Contents   ${Description}  ${Name}   1

    ${channel} =  Get Process Channel
    Check Capn Output For A Single Message   ${channel}  eventType=start  pathname.pathname=${touchPath}  ownerUserSID.userid=0
    Check Capn Output For A Single Message   ${channel}  eventType=start  pathname.pathname=${rmPath}  ownerUserSID.userid=0
    Check Capn Output For A Single Message   ${channel}  eventType=start  pathname.pathname=${pingPath}  ownerUserSID.userid=0

Audit Plugin Processes Wget Example And Produces XML Event
    Start Subscriber

    Run AuditPlugin Against Example  call_wget_on_a_url

    ${Description}=  Set Variable  "Unsafe process was started"
    ${Name}=  Set Variable  "ATTENTION: unsafe process was started"

    Wait Until Keyword Succeeds
    ...  3 secs
    ...  1 secs
    ...  SAV Event File Exists And Contains Correct Contents   ${Description}  ${Name}   1

    ${channel} =  Get Process Channel
    Check Capn Output For A Single Message   ${channel}  eventType=start  pathname.pathname=/bin/cp  ownerUserSID.userid=0

Audit Plugin Processes Wget Example With Equals In The Argument And Produces XML Event
    Start Subscriber

    Run AuditPlugin Against Example  wget_with_special_chars

    ${Description}=  Set Variable  "Unsafe process was started"
    ${Name}=  Set Variable  "ATTENTION: unsafe process was started"

    Wait Until Keyword Succeeds
    ...  3 secs
    ...  1 secs
    ...  SAV Event File Exists And Contains Correct Contents   ${Description}  ${Name}   1

    ${channel} =  Get Process Channel
    Check Capn Output For A Single Message   ${channel}  eventType=start  pathname.pathname=/usr/bin/wget  procTitle=wget a0=2

Audit Plugin Processes Wget Example With Really Long Args And Produces XML Event
    Start Subscriber

    Run AuditPlugin Against Example  execute_wget_with_very_long_arg_string

    ${Description}=  Set Variable  "Unsafe process was started"
    ${Name}=  Set Variable  "ATTENTION: unsafe process was started"

    Wait Until Keyword Succeeds
    ...  3 secs
    ...  1 secs
    ...  SAV Event File Exists And Contains Correct Contents   ${Description}  ${Name}   1

    ${channel} =  Get Process Channel
    Check Capn Output For A Single Message   ${channel}  eventType=start  pathname.pathname=/bin/cp  ownerUserSID.userid=0

Audit Plugin Processes Wget Example With 2000 Char Long Args And Produces XML Event
    Start Subscriber

    Run AuditPlugin Against Example  execute_wget_with_2000_long_arg_string

    ${Description}=  Set Variable  "Unsafe process was started"
    ${Name}=  Set Variable  "ATTENTION: unsafe process was started"

    Wait Until Keyword Succeeds
    ...  3 secs
    ...  1 secs
    ...  SAV Event File Exists And Contains Correct Contents   ${Description}  ${Name}   1

    ${channel} =  Get Process Channel
    Check Capn Output For A Single Message   ${channel}  eventType=start  pathname.pathname=/bin/cp  ownerUserSID.userid=0

Audit Plugin Processes Wget Run From Directory With A Quote In Its Name And Produces XML Event
    Start Subscriber

    Run AuditPlugin Against Example  wget_in_dir_with_quote

    ${Description}=  Set Variable  "Unsafe process was started"
    ${Name}=  Set Variable  "ATTENTION: unsafe process was started"

    Wait Until Keyword Succeeds
    ...  3 secs
    ...  1 secs
    ...  SAV Event File Exists And Contains Correct Contents   ${Description}  ${Name}   1

    ${channel} =  Get Process Channel
    Check Capn Output For A Single Message   ${channel}  eventType=start  pathname.pathname=/home/vagrant/bla'h/wget  procTitle=./wget

Audit Plugin Processes Wget Example In Really Deep Directory Produces XML Event
    Start Subscriber

    Run AuditPlugin Against Example  deep_path_fake_wget_process_run  amazon_linux

    ${Description}=  Set Variable  "Unsafe process was started"
    ${Name}=  Set Variable  "ATTENTION: unsafe process was started"

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SAV Event File Exists And Contains Correct Contents   ${Description}  ${Name}   1

    ${channel} =  Get Process Channel
    Check Capn Output For A Single Message   ${channel}  eventType=start  pathname.pathname=<too_long>  ownerUserSID.sid=0  cmdLine=./wget test
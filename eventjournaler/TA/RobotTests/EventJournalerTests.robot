*** Settings ***
Resource  EventJournalerResources.robot
Suite Setup  Setup
Test Teardown   Event Journaler Teardown
Suite Teardown  Uninstall Base

*** Test Cases ***

Event Journal Files Are Not Deleted When Downgrading
    ${filePath} =  set Variable  ${EVENT_JOURNALER_DATA_STORE}/producer/threatEvents/threatEvents-00001-00002-12092029-10202002
    Create Journal Test File  ${filePath}
    Downgrade Event Journaler

    # prove that the file is not removed when performing a downgrade
    Should Exist  ${filePath}.bin
    ${leftover_directory_contents} =  List Directory  ${SOPHOS_INSTALL}/plugins/eventjournaler
    Log  ${leftover_directory_contents}
    Should Be Equal As Integers  1  ${leftover_directory_contents.__len__()}
    Should Contain  ${leftover_directory_contents}  data

    Install Event Journaler Directly from SDDS
    # Journal File may be compressed, so just check we have at 1 file in the event journal
    ${files} =  List Files In Directory  ${EVENT_JOURNALER_DATA_STORE}/producer/threatEvents/
    Log  ${files}
    Should Be Equal As Integers  1  ${files.__len__()}

Event Journaler Can Receive Events
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Should Exist   ${EVENT_JOURNALER_LOG_PATH}
    ${mark} =  Mark File  ${EVENT_JOURNALER_LOG_PATH}

    Publish Threat Event

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check Journal Contains Detection Event With Content   {"threatName":"EICAR-AV-Test","threatPath":"/home/admin/eicar.com"}

Event Journaler Restarts Subscriber After Socket Removed
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Should Exist   ${EVENT_JOURNALER_LOG_PATH}
    ${mark} =  Mark File  ${EVENT_JOURNALER_LOG_PATH}

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Should Exist  ${SOPHOS_INSTALL}/var/ipc/events.ipc

    # Delete socket file, subscriber should exit and Journaler should restart it.
    Remove Subscriber Socket

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Marked Event Journaler Log Contains  The subscriber socket has been unexpectedly removed   ${mark}

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Marked Event Journaler Log Contains  Subscriber not running, restarting it   ${mark}

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Marked Event Journaler Log Contains  Subscriber restart called   ${mark}


Event Journaler Can Receive Events From Multiple Publishers
    [Teardown]  Custom Teardown For Tests With Publishers Running In Background
    ${mark} =  Mark File  ${EVENT_JOURNALER_LOG_PATH}
    Publish Threat Events In Background Process   {"threatName":"EICAR-AV-Test","test data 1"}  1
    Publish Threat Events In Background Process   {"threatName":"EICAR-AV-Test","test data 2"}  1
    Publish Threat Events In Background Process   {"threatName":"EICAR-AV-Test","test data 3"}  1

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Get Three Events For Multiple Publishers Test

Check Closed And Compressed Journal Files Can Still Be Read
    Publish Threat Event With Specific Data  {"threatName":"EICAR-AV-Test","test data 1"}
    assert_expected_file_in_directory  ${EVENT_JOURNALER_DATA_STORE}/SophosSPL/Detections  expected_closed_files=${0}  expected_open_files=${1}  expected_compressed_files=${0}
    Check Journal Contains X Detection Events  1
    ${file_1} =  get_open_journal_file_from_directory  ${EVENT_JOURNALER_DATA_STORE}/SophosSPL/Detections
    Change Capnproto Version Of Journal File  ${file_1}

    Restart Event Journaler
    assert_expected_file_in_directory  ${EVENT_JOURNALER_DATA_STORE}/SophosSPL/Detections  expected_closed_files=${0}  expected_open_files=${0}  expected_compressed_files=${1}
    Publish Threat Event With Specific Data  {"threatName":"EICAR-AV-Test","test data 2"}
    assert_expected_file_in_directory  ${EVENT_JOURNALER_DATA_STORE}/SophosSPL/Detections  expected_closed_files=${0}  expected_open_files=${1}  expected_compressed_files=${1}
    Check Journal Contains X Detection Events  2

    ${file_2} =  get_open_journal_file_from_directory  ${EVENT_JOURNALER_DATA_STORE}/SophosSPL/Detections
    Change Capnproto Version Of Journal File  ${file_2}

    Publish Threat Event With Specific Data  {"threatName":"EICAR-AV-Test","test data 3"}

    Get Three Events For Multiple Publishers Test
    assert_expected_file_in_directory  ${EVENT_JOURNALER_DATA_STORE}/SophosSPL/Detections  expected_closed_files=${1}  expected_open_files=${1}  expected_compressed_files=${1}
    Check Journal Contains X Detection Events  3

    # Removing a file reduces detection events by 1
    Remove Journal File In Directory And Expect Subsequent Journal Detections To Be N  ${EVENT_JOURNALER_DATA_STORE}/SophosSPL/Detections  2
    Expect N Files In Directory  2  ${EVENT_JOURNALER_DATA_STORE}/SophosSPL/Detections
    Remove Journal File In Directory And Expect Subsequent Journal Detections To Be N  ${EVENT_JOURNALER_DATA_STORE}/SophosSPL/Detections  1
    Expect N Files In Directory  1  ${EVENT_JOURNALER_DATA_STORE}/SophosSPL/Detections
    Remove Journal File In Directory And Expect Subsequent Journal Detections To Be N  ${EVENT_JOURNALER_DATA_STORE}/SophosSPL/Detections  0
    Expect N Files In Directory  0  ${EVENT_JOURNALER_DATA_STORE}/SophosSPL/Detections

Event Journaler Can Receive Many Events From Publisher
    FOR  ${i}  IN RANGE  100
        Publish Threat Event With Specific Data  {"threatName":"EICAR-AV-Test","test data":"${i}"}
    END

    ${all_events} =  Read All Detection Events From Journal
    FOR  ${i}  IN RANGE  100
        should contain   ${all_events}  {"threatName":"EICAR-AV-Test","test data":"${i}"}
    END

Event Journaler Can Receive Malformed Events From Publisher without crashing
    [Tags]  DO
    Publish Threat Event With Specific Data  {}
    Publish Threat Event With Specific Data  ""
    Publish Threat Event With Specific Data  {nisndisodwkpo
    ${result}=   Get File  ${EXAMPLE_DATA_PATH}/Detections-0000000000000001-0000000000000003-132729637080000000-132729637110000000.bin
    Publish Threat Event With Specific Data  {"threatName":"EICAR-AV-Test","test data":"${result.stdout}","filepath":"/mnt/dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"}
    Check Event Journaler Executable Running

Event Journaler Can compress Files
    ${filePath} =  set Variable  ${EVENT_JOURNALER_DATA_STORE}/producer/threatEvents/threatEvents-00001-00002-12092029-10202002
    Create Journal Test File  ${filePath}
    Restart Event Journaler
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  File Should exist  ${filePath}.xz
    File Should not exist  ${filePath}.bin
    File Exists With Permissions  ${filepath}.xz  sophos-spl-user  sophos-spl-group  -rw-r-----

Event Journaler Can Remove Empty Files
    ${filePath} =  Set Variable  ${EVENT_JOURNALER_DATA_STORE}/producer/threatEvents/threatEvents-00001-00002-12092029-10202002
    Create Binary File   ${filePath}.bin  ${EMPTY}
    Run Process  chown  sophos-spl-user:sophos-spl-group  -R  ${EVENT_JOURNALER_DATA_STORE}/
    Restart Event Journaler
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  File Should Not Exist  ${filePath}.bin
    File Should Not Exist  ${filePath}.xz

Event Journaler Can Fix Truncated Files
    FOR  ${i}  IN RANGE  3
        Publish Threat Event
    END

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check Journal Contains X Detection Events  3

    ${files} =  List Files In Directory  ${EVENT_JOURNALER_DATA_STORE}/SophosSPL/Detections  Detections-*.bin
    ${numFiles} =  Get Length  ${files}
    Should Be Equal As Integers  ${numFiles}  1
    Run Process  truncate  -s  -13  ${EVENT_JOURNALER_DATA_STORE}/SophosSPL/Detections/${files[0]}

    Restart Event Journaler

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check Journal Contains X Detection Events  2

*** Keywords ***
Setup
    Install Base For Component Tests
    Install Event Journaler Directly from SDDS
    Run Shell Process  chmod a+x ${EVENT_PUB_SUB_TOOL}  OnError=Failed to chmod EventPubSub binary tool   timeout=3s
    Run Shell Process  chmod a+x ${EVENT_READER_TOOL}  OnError=Failed to chmod JournalReader binary tool   timeout=3s

Custom Teardown For Tests With Publishers Running In Background
    Event Journaler Teardown
    Terminate All Processes

Get Three Events For Multiple Publishers Test
    ${all_events} =  Read All Detection Events From Journal
    should contain   ${all_events}  {"threatName":"EICAR-AV-Test","test data 1"}
    should contain   ${all_events}  {"threatName":"EICAR-AV-Test","test data 2"}
    should contain   ${all_events}  {"threatName":"EICAR-AV-Test","test data 3"}

File Exists With Permissions
    [Arguments]  ${file}  ${user}  ${group}  ${permissions}
    ${result} =  Run Process  stat  -c  "%U %G %A"  ${file}
    @{words} =  Split String  ${result.stdout.strip('"')}
    Should Be Equal As Strings  ${words[0]}  ${user}
    Should Be Equal As Strings  ${words[1]}  ${group}
    Should Be Equal As Strings  ${words[2]}  ${permissions}

Change Capnproto Version Of Journal File
    [Arguments]  ${journal_file_full_path}
    ${current_capn_version} =  Set Variable  0.8.0
    ${target_capn_version} =  Set Variable  0.7.0
    ${result} =  Run Process  sed  -i  s/${current_capn_version}/${target_capn_version}/  ${journal_file_full_path}
    Log  ${result.stdout}
    Log  ${result.stderr}
    ${result} =  Run Process  grep  0.7.0  ${journal_file_full_path}
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Integers  0  ${result.rc}

Expect N Files In Directory
    [Arguments]  ${n}  ${directory}
    ${files} =  List Files In Directory  ${directory}
    Log  ${files}
    Should Be Equal As Integers  ${n}  ${files.__len__()}

Remove File In Directory
    [Arguments]  ${directory}
    ${filenames} =  list files in directory  ${directory}
    remove file  ${directory}/${filenames[0]}

Remove Journal File In Directory And Expect Subsequent Journal Detections To Be N
    [Arguments]  ${directory}  ${N}
    Remove File In Directory  ${directory}
    Check Journal Contains X Detection Events  ${N}

Create Journal Test File
    [Arguments]  ${filePath}
    Copy File  ${EXAMPLE_DATA_PATH}/Detections-0000000000000001-0000000000000003-132729637080000000-132729637110000000.bin  ${filePath}.bin
    Run Process  chown  sophos-spl-user:sophos-spl-group  -R  ${EVENT_JOURNALER_DATA_STORE}/

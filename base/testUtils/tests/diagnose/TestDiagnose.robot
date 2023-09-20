*** Settings ***

Library     ${COMMON_TEST_LIBS}/LogUtils.py
Library     ${LIBS_DIRECTORY}/DiagnoseUtils.py
Library     ${LIBS_DIRECTORY}/OSUtils.py

Library     Process
Library     OperatingSystem
Library     Collections

Resource    ../edr_plugin/EDRResources.robot
Resource    ../liveresponse_plugin/LiveResponseResources.robot
Resource    DiagnoseResources.robot
Resource    ../runtimedetections_plugin/RuntimeDetectionsResources.robot
Resource    ../ra_plugin/ResponseActionsResources.robot

Suite Setup  Require Fresh Install
Suite Teardown  Ensure Uninstalled

Test Setup      Should Exist  ${SOPHOS_INSTALL}/bin/sophos_diagnose
Test Teardown   Teardown

Force Tags  DIAGNOSE

*** Test Cases ***

Diagnose Tool Gathers Logs When Run From Installation
    [Tags]  TAP_PARALLEL4    SMOKE
    Mimic Base Component Files  ${SOPHOS_INSTALL}
    Wait Until Created  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcs_envelope.log     20 seconds
    create file    ${SOPHOS_INSTALL}/base/update/var/updatescheduler/installed_features.json
    Create Directory  ${TAR_FILE_DIRECTORY}
    ${retcode} =  Run Diagnose    ${SOPHOS_INSTALL}/bin/     ${TAR_FILE_DIRECTORY}

    Should Be Equal As Integers   ${retcode}  0

    # Check diagnose tar created
    ${Files} =  List Files In Directory  ${TAR_FILE_DIRECTORY}/
    ${fileCount} =    Get length    ${Files}
    Should Be Equal As Numbers  ${fileCount}  1
    Should Contain    ${Files[0]}    sspl-diagnose
    Should Not Contain   ${Files}  BaseFiles
    Should Not Contain   ${Files}  SystemFiles
    Should Not Contain   ${Files}  PluginFiles

    ${folder}=  Fetch From Left   ${Files[0]}   .tar.gz
    Set Suite Variable  ${DiagnoseOutput}  ${folder}
    # Untar diagnose tar to check contents
    Create Directory  ${UNPACK_DIRECTORY}
    ${result} =   Run Process   tar    xzf    ${TAR_FILE_DIRECTORY}/${Files[0]}    -C    ${UNPACK_DIRECTORY}/
    Should Be Equal As Strings   ${result.rc}  0

    Check Diagnose Base Output
    File Should exist    ${UNPACK_DIRECTORY}/${DiagnoseOutput}/BaseFiles/installed_features.json
    Check Diagnose Output For System Command Files
    Check Diagnose Output For System Files

    ${contents} =  Get File  /tmp/diagnose.log
    Should Not Contain  ${contents}  error  ignore_case=True
    Should Not Contain  ${contents}  Running command: 'systemctl status sophos-spl-update' failed to complete with
    Should Contain  ${contents}   Created tarfile: ${Files[0]} in directory ${TAR_FILE_DIRECTORY}

Diagnose Tool Gathers Logs When Run From Systemctl
    [Tags]   SMOKE
    Mimic Base Component Files  ${SOPHOS_INSTALL}
    Wait Until Created  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcs_envelope.log     20 seconds


    Run Process  ${SOPHOS_INSTALL}/bin/sophos_diagnose  --remote

    # Check diagnose tar created
    ${Files} =  List Files In Directory  ${SOPHOS_INSTALL}/base/remote-diagnose/output/
    ${fileCount} =    Get length    ${Files}
    Should Be Equal As Numbers  ${fileCount}  1
    Should Contain    ${Files[0]}    sspl.zip
    Should Not Contain   ${Files}  BaseFiles
    Should Not Contain   ${Files}  SystemFiles
    Should Not Contain   ${Files}  PluginFiles

    ${folder}=  Fetch From Left   ${Files[0]}   .tar.gz
    Set Suite Variable  ${DiagnoseOutput}  /
    # Untar diagnose tar to check contents
    Create Directory  ${UNPACK_DIRECTORY}
    ${result} =   Run Process   unzip    ${SOPHOS_INSTALL}/base/remote-diagnose/output/${Files[0]}    -d    ${UNPACK_DIRECTORY}/
    Should Be Equal As Strings   ${result.rc}  0
    # Check diagnose tar created
    ${Files} =  List Directory  ${UNPACK_DIRECTORY}/
    ${fileCount} =    Get length    ${Files}
    Should Be Equal As Numbers  ${fileCount}  1
    Should Contain    ${Files[0]}    sspl-diagnose


    Set Suite Variable  ${DiagnoseOutput}   ${Files[0]}

    Check Diagnose Base Output
    Check Diagnose Output For System Command Files
    Check Diagnose Output For System Files

Diagnose Tool Gathers LR Logs When Run From Installation
    [Tags]   LIVE_RESPONSE
    Wait Until Created  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcs_envelope.log     20 seconds

    Create Directory  ${TAR_FILE_DIRECTORY}

    Install Live Response Directly
    Mimic LR Component Files   ${SOPHOS_INSTALL}

    ${retcode} =  Run Diagnose    ${SOPHOS_INSTALL}/bin/     ${TAR_FILE_DIRECTORY}
    Should Be Equal As Integers   ${retcode}  0

    # Check diagnose tar created
    ${Files} =  List Files In Directory  ${TAR_FILE_DIRECTORY}/
    ${fileCount} =    Get length    ${Files}
    Should Be Equal As Numbers  ${fileCount}  1
    Should Contain    ${Files[0]}    sspl-diagnose
    Should Not Contain   ${Files}  BaseFiles
    Should Not Contain   ${Files}  SystemFiles
    Should Not Contain   ${Files}  PluginFiles


    ${folder}=  Fetch From Left   ${Files[0]}   .tar.gz
    Set Suite Variable  ${DiagnoseOutput}  ${folder}
    # Untar diagnose tar to check contents
    Create Directory  ${UNPACK_DIRECTORY}
    ${result} =   Run Process   tar    xzf    ${TAR_FILE_DIRECTORY}/${Files[0]}    -C    ${UNPACK_DIRECTORY}/
    Should Be Equal As Strings   ${result.rc}  0

    Check Diagnose Output For Additional LR Plugin Files
    Check Diagnose Output For System Command Files
    Check Diagnose Output For System Files

    ${contents} =  Get File  /tmp/diagnose.log
    Should Not Contain  ${contents}  error  ignore_case=True
    Should Contain  ${contents}   Created tarfile: ${Files[0]} in directory ${TAR_FILE_DIRECTORY}

Diagnose Tool Gathers RuntimeDetections Logs When Run From Installation
    [Tags]  RUNTIMEDETECTIONS_PLUGIN
    Wait Until Created  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcs_envelope.log     20 seconds

    Create Directory  ${TAR_FILE_DIRECTORY}

    Install RuntimeDetections Directly

    Wait Until Keyword Succeeds
        ...   10 secs
        ...   1 secs
        ...   File Should Exist  ${SOPHOS_INSTALL}/plugins/runtimedetections/log/runtimedetections.log

    ${retcode} =  Run Diagnose    ${SOPHOS_INSTALL}/bin/     ${TAR_FILE_DIRECTORY}
    Should Be Equal As Integers   ${retcode}  0

    # Check diagnose tar created
    ${Files} =  List Files In Directory  ${TAR_FILE_DIRECTORY}/
    ${fileCount} =    Get length    ${Files}
    Should Be Equal As Numbers  ${fileCount}  1
    ${folder}=  Fetch From Left   ${Files[0]}   .tar.gz
    Set Suite Variable  ${DiagnoseOutput}  ${folder}
    # Untar diagnose tar to check contents
    Create Directory  ${UNPACK_DIRECTORY}
    ${result} =   Run Process   tar    xzf    ${TAR_FILE_DIRECTORY}/${Files[0]}    -C    ${UNPACK_DIRECTORY}/
    Should Be Equal As Strings   ${result.rc}  0

    Check Diagnose Output For Additional RuntimeDetections Plugin Files
    Check Diagnose Output For System Command Files
    Check Diagnose Output For System Files

    ${contents} =  Get File  /tmp/diagnose.log
    Should Not Contain  ${contents}  error  ignore_case=True
    Should Contain  ${contents}   Created tarfile: ${Files[0]} in directory ${TAR_FILE_DIRECTORY}

Diagnose Tool Gathers Response actions Logs When Run From Installation
    [Tags]  RESPONSE_ACTIONS_PLUGIN  TAP_PARALLEL4
    Wait Until Created  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcs_envelope.log     20 seconds

    Create Directory  ${TAR_FILE_DIRECTORY}

    Install Response Actions Directly

    Wait Until Keyword Succeeds
        ...   10 secs
        ...   1 secs
        ...   File Should Exist  ${RESPONSE_ACTIONS_LOG_PATH}

    ${retcode} =  Run Diagnose    ${SOPHOS_INSTALL}/bin/     ${TAR_FILE_DIRECTORY}
    Should Be Equal As Integers   ${retcode}  0

    # Check diagnose tar created
    ${Files} =  List Files In Directory  ${TAR_FILE_DIRECTORY}/
    ${fileCount} =    Get length    ${Files}
    Should Be Equal As Numbers  ${fileCount}  1
    ${folder}=  Fetch From Left   ${Files[0]}   .tar.gz
    Set Suite Variable  ${DiagnoseOutput}  ${folder}
    # Untar diagnose tar to check contents
    Create Directory  ${UNPACK_DIRECTORY}
    ${result} =   Run Process   tar    xzf    ${TAR_FILE_DIRECTORY}/${Files[0]}    -C    ${UNPACK_DIRECTORY}/
    Should Be Equal As Strings   ${result.rc}  0

    Check Diagnose Output For RA Plugin logs

    ${contents} =  Get File  /tmp/diagnose.log
    Should Not Contain  ${contents}  error  ignore_case=True
    Should Contain  ${contents}   Created tarfile: ${Files[0]} in directory ${TAR_FILE_DIRECTORY}

Diagnose Tool Help Text
    [Tags]    TAP_PARALLEL3
    ${result} =   Run Process   ${SOPHOS_INSTALL}/bin/sophos_diagnose  --help
    Should Be Equal   ${result.stderr}   Expected Usage: ./sophos_diagnose <path_to_output_directory>
    Should Be Equal As Integers   ${result.rc}  0

Diagnose Tool Bad Input Fails
    [Tags]    TAP_PARALLEL3
    ${result} =   Run Process   ${SOPHOS_INSTALL}/bin/sophos_diagnose  asdasdasdasdasd
    Should Be Equal As Integers    ${result.rc}  3
    Should Contain   ${result.stderr}   Cause: No such file or directory
    Should Not Exist  ${UNPACK_DIRECTORY}

Diagnose Tool More Than Expected Input Fails
    [Tags]    TAP_PARALLEL3
    ${result} =   Run Process   ${SOPHOS_INSTALL}/bin/sophos_diagnose  asdasdasdasdasd    dddddd
    Should Be Equal As Integers    ${result.rc}  1
    Should Be Equal   ${result.stderr}    	Expecting only one parameter got 2
    Should Not Exist  ${UNPACK_DIRECTORY}

Diagnose Tool No Input Creates Output Locally
    [Tags]    TAP_PARALLEL3
    ${result} =   Run Process   ${SOPHOS_INSTALL}/bin/sophos_diagnose
    Log  ${result.stdout}
    Should Be Equal As Integers  ${result.rc}  0
    File Should Exist  sspl-diagnose_*.tar.gz
    Remove File  sspl-diagnose_*.tar.gz

Diagnose Tool Run Twice Creates Two Tar Files
    [Tags]    TAP_PARALLEL3
    Create Directory  ${TAR_FILE_DIRECTORY}

    ${result} =   Run Process   ${SOPHOS_INSTALL}/bin/sophos_diagnose    ${TAR_FILE_DIRECTORY}
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Integers  ${result.rc}  0
    # Check diagnose tar created
    ${Files} =  List Files In Directory  ${TAR_FILE_DIRECTORY}
    ${fileCount} =    Get length    ${Files}
    Should Be Equal As Numbers  ${fileCount}  1
    Should Contain    ${Files[0]}    sspl-diagnose_
    Should Contain    ${Files[0]}    .tar.gz
    Move file    ${TAR_FILE_DIRECTORY}/${Files}[0]    ${TAR_FILE_DIRECTORY}/old_diagnose.tar
    ${result} =   Run Process   ${SOPHOS_INSTALL}/bin/sophos_diagnose    ${TAR_FILE_DIRECTORY}
    Log  ${result.stdout}
    Log  ${result.stderr}
    # Check diagnose tar created
    ${Files} =  List Files In Directory  ${TAR_FILE_DIRECTORY}
    ${fileCount} =    Get length    ${Files}
    Should Be Equal As Numbers  ${fileCount}  2

    Should Contain    ${Files[1]}    sspl-diagnose_
    Should Contain    ${Files[1]}    .tar.gz


Diagnose Tool Deletes Temp Directory if tar fails
    [Tags]    TAP_PARALLEL3
    Create Directory  ${TAR_FILE_DIRECTORY}
    Create Directory  ${TAR_FILE_DIRECTORY}/bin
    Create Symlink  /bin/false  ${TAR_FILE_DIRECTORY}/bin/tar
    ${result} =   Run Process   ${SOPHOS_INSTALL}/bin/sophos_diagnose  ${TAR_FILE_DIRECTORY}  env:PATH=${TAR_FILE_DIRECTORY}/bin:%{PATH}
    Should Not Be Equal As Integers    ${result.rc}  0
    Log    ${result.stderr}
    File Should Not Exist  ${TAR_FILE_DIRECTORY}/sspl-diagnose_*.tar.gz

    ## We created bin so we need to delete it
    Remove Directory  ${TAR_FILE_DIRECTORY}/bin  true

    ${Files} =  List Directory  ${TAR_FILE_DIRECTORY}
    ${fileCount} =    Get length    ${Files}
    Should Be Equal As Numbers  ${fileCount}  0


Diagnose Tool Sets Correct Directory Permissions
    [Tags]    TAP_PARALLEL3
    Create Directory  ${TAR_FILE_DIRECTORY}
    Run Process  chmod  700  ${TAR_FILE_DIRECTORY}
    ${retcode} =  Run Diagnose    ${SOPHOS_INSTALL}/bin/     ${TAR_FILE_DIRECTORY}
    LogUtils.Dump Log    ${TAR_FILE_DIRECTORY}/diagnose.log
    Should Be Equal As Integers   ${retcode}  0

    ${permissions} =  Run Process  ls  -l    ${TAR_FILE_DIRECTORY}
    Log  ${permissions.stdout}
    ${result} =  Check File Permissions  ${TAR_FILE_DIRECTORY}
    Should Contain  ${result}   Correct Permissions

Diagnose Tool Does Not Gather JournalCtl Data Older Than 10 Days
    [Tags]    TAP_PARALLEL3
    Create Directory  ${TAR_FILE_DIRECTORY}
    Run Process  chmod  700  ${TAR_FILE_DIRECTORY}
    ${result} =   Run Process   ${SOPHOS_INSTALL}/bin/sophos_diagnose    ${TAR_FILE_DIRECTORY}
    Log  ${result.stdout}
    Should Be Equal As Integers  ${result.rc}  0     msg=Diagnose Failed with message: ${result.stderr}

    # Untar diagnose tar to check contents
    ${Files} =  List Files In Directory  ${TAR_FILE_DIRECTORY}

    ${folder}=  Fetch From Left   ${Files[0]}   .tar.gz

    Create Directory  ${UNPACK_DIRECTORY}
    ${result} =   Run Process   tar    xzf    ${TAR_FILE_DIRECTORY}/${Files[0]}    -C    ${UNPACK_DIRECTORY}/
    Should Be Equal As Strings   ${result.rc}  0

    Check Journalclt Files Do Not Have Old Entries   ${UNPACK_DIRECTORY}/${folder}/SystemFiles    days=11

Diagnose Tool Fails Due To Full Disk Partition And Should Not Generate Uncaught Exception
    [Tags]  SMOKE  TAP_PARALLEL3
    # Create 6M Log File
    Create Log File Of Specific Size  ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log  6291456

    Run Process   touch   /tmp/5mbarea
    Run Process   truncate   -s   5M   /tmp/5mbarea
    Run Process   mke2fs   -t   ext4   -F   /tmp/5mbarea
    Run Process   mkdir   /tmp/up
    Run Process   mount   /tmp/5mbarea   /tmp/up

    ${result} =   Run Process   ${SOPHOS_INSTALL}/bin/sophos_diagnose  /tmp/up
    Log    ${result.stderr}
    Log    ${result.stdout}

    Should Be Equal As Integers    ${result.rc}  3

    Should Not Contain   ${result.stderr}    	Uncaught std::exception
    Should Contain   ${result.stderr}    failed to complete writing to file, check space available on device

Diagnose Tool Fails Due To Read Only Mount And Should Not Generate Uncaught Exception
    [Tags]    TAP_PARALLEL3
    Run Process   touch   /tmp/5mbarea
    Run Process   truncate   -s   5M   /tmp/5mbarea
    Run Process   mke2fs   -t   ext4   -F   /tmp/5mbarea
    Run Process   mkdir   /tmp/up
    Run Process   mount   -r  /tmp/5mbarea   /tmp/up

    ${result} =   Run Process   ${SOPHOS_INSTALL}/bin/sophos_diagnose  /tmp/up
    Should Be Equal As Integers    ${result.rc}  3
    Log    ${result.stderr}
    Should Not Contain   ${result.stderr}    	Uncaught std::exception
    Should Contain   ${result.stderr}   File system error: Failed to create directory
    Should Contain   ${result.stderr}   Cause: Read-only file system


Diagnose Tool Overwrite Handles Files Of Same Name In Different Directories
    [Tags]    TAP_PARALLEL3
    Wait Until Created  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcs_envelope.log     20 seconds

    Mimic Base Component Files  ${SOPHOS_INSTALL}
    Create File  ${BASE_LOGS_DIR}/a.log  text
    Create File  ${BASE_LOGS_DIR}/sophosspl/a.log  text
    Create File  ${EDR_DIR}/log/plugin.log  text
    Create File  ${EDR_DIR}/etc/plugin.log  text

    Create Directory  ${TAR_FILE_DIRECTORY}
    ${retcode} =  Run Diagnose    ${SOPHOS_INSTALL}/bin/     ${TAR_FILE_DIRECTORY}
    Should Be Equal As Integers   ${retcode}  0

    ${Files} =  List Files In Directory  ${TAR_FILE_DIRECTORY}/
    ${folder}=  Fetch From Left   ${Files[0]}   .tar.gz
    Set Suite Variable  ${DiagnoseOutput}  ${folder}
    # Untar diagnose tar to check contents
    Create Directory  ${UNPACK_DIRECTORY}
    ${result} =   Run Process   tar    xzf    ${TAR_FILE_DIRECTORY}/${Files[0]}    -C    ${UNPACK_DIRECTORY}/
    Should Be Equal As Strings   ${result.rc}  0

    Check Diagnose Base Output
    Check Diagnose Output For System Command Files
    Check Diagnose Output For System Files

    # Check for a.log and a.log.1, incase there is a clash we append a suffix.
    ${base_files} =  List Files In Directory  ${UNPACK_DIRECTORY}/${DiagnoseOutput}/BaseFiles
    Should Contain  ${base_files}    a.log
    Should Contain  ${base_files}    a.log.1

    # Check for the two plugin.log files, they should be in seperate directories.
    ${plugin_files} =  List Files In Directory  ${UNPACK_DIRECTORY}/${DiagnoseOutput}/PluginFiles/edr
    Should Contain  ${plugin_files}    plugin.log
    ${plugin_files} =  List Files In Directory  ${UNPACK_DIRECTORY}/${DiagnoseOutput}/PluginFiles/edr/etc
    Should Contain  ${plugin_files}    plugin.log

    ${contents} =  Get File  /tmp/diagnose.log
    log  ${contents}
    Should Not Contain  ${contents}  error  ignore_case=True


    Should Contain  ${contents}   Created tarfile: ${Files[0]} in directory ${TAR_FILE_DIRECTORY}


Diagnose Tool Gathers EDR Logs When Run From Installation
    #WARNING this test should be the last in the suite to avoid watchdog going down due to the defect LINUXDAR-3732
    [Tags]    EDR_PLUGIN
    Wait Until Created  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcs_envelope.log     20 seconds

    Create Directory  ${TAR_FILE_DIRECTORY}

    Install EDR Directly
    Wait Until Keyword Succeeds
        ...   10 secs
        ...   1 secs
        ...   File Should Exist  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf

    Copy File  ${SUPPORT_FILES}/xdr-query-packs/error-queries.conf  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf
    Copy File  ${SUPPORT_FILES}/xdr-query-packs/error-queries.conf  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.mtr.conf
    Wait Until Keyword Succeeds
        ...   10 secs
        ...   1 secs
        ...   File Should Exist  ${SOPHOS_INSTALL}/plugins/edr/log/edr.log
    Wait Until Keyword Succeeds
        ...   10 secs
        ...   1 secs
        ...   File Should Exist  ${SOPHOS_INSTALL}/plugins/edr/log/osqueryd.results.log
    ${retcode} =  Run Diagnose    ${SOPHOS_INSTALL}/bin/     ${TAR_FILE_DIRECTORY}
    Should Be Equal As Integers   ${retcode}  0

    # Check diagnose tar created
    ${Files} =  List Files In Directory  ${TAR_FILE_DIRECTORY}/
    ${fileCount} =    Get length    ${Files}
    Should Be Equal As Numbers  ${fileCount}  1

    ${folder}=  Fetch From Left   ${Files[0]}   .tar.gz
    Set Suite Variable  ${DiagnoseOutput}  ${folder}
    # Untar diagnose tar to check contents
    Create Directory  ${UNPACK_DIRECTORY}
    ${result} =   Run Process   tar    xzf    ${TAR_FILE_DIRECTORY}/${Files[0]}    -C    ${UNPACK_DIRECTORY}/
    Should Be Equal As Strings   ${result.rc}  0

    log file  /tmp/diagnose.log
    Check Diagnose Output For Additional EDR Plugin File
    Check Diagnose Output For System Command Files
    Check Diagnose Output For System Files

    ${contents} =  Get File  /tmp/diagnose.log
    Should Not Contain  ${contents}  error  ignore_case=True
    Should Contain  ${contents}   Created tarfile: ${Files[0]} in directory ${TAR_FILE_DIRECTORY}

*** Settings ***

Library     ${libs_directory}/LogUtils.py
Library     ${libs_directory}/DiagnoseUtils.py
Library     ${libs_directory}/OSUtils.py

Library     Process
Library     OperatingSystem

Resource    ../management_agent-audit_plugin/AuditPluginResources.robot
Resource    ../management_agent-event_processor/EventProcessorResources.robot
Resource    DiagnoseResources.robot

Suite Setup  Require Fresh Install
Suite Teardown  Ensure Uninstalled

Test Setup      Should Exist  ${SOPHOS_INSTALL}/bin/sophos_diagnose
Test Teardown   Teardown

Default Tags  DIAGNOSE

*** Test Cases ***

Diagnose Tool Gathers Logs When Run From Installation
    [Tags]  EVENT_PLUGIN  AUDIT_PLUGIN  DIAGNOSE
    Install Audit Plugin Directly
    Install EventProcessor Plugin Directly

    Wait Until Created  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcs_envelope.log     20 seconds

    Create Directory  ${TAR_FILE_DIRECTORY}
    ${retcode} =  Run Diagnose    ${SOPHOS_INSTALL}/bin/     ${TAR_FILE_DIRECTORY}
    LogUtils.Dump Log    /tmp/diagnose.log
    Should Be Equal As Integers   ${retcode}  0

    # Check diagnose tar created
    ${Files} =  List Files In Directory  ${TAR_FILE_DIRECTORY}/
    ${fileCount} =    Get length    ${Files}
    Should Be Equal As Numbers  ${fileCount}  1
    Should Contain    ${Files[0]}    sspl-diagnose
    Should Not Contain   ${Files}  BaseFiles
    Should Not Contain   ${Files}  SystemFiles
    Should Not Contain   ${Files}  PluginFiles

    # Untar diagnose tar to check contents
    Create Directory  ${UNPACK_DIRECTORY}
    ${result} =   Run Process   tar    xzf    ${TAR_FILE_DIRECTORY}/${Files[0]}    -C    ${UNPACK_DIRECTORY}/
    Should Be Equal As Strings   ${result.rc}  0

    Check Diagnose Base Output
    Check Diagnose Output For Plugin logs
    Check Diagnose Output For System Command Files
    Check Diagnose Output For System Files

    ${contents} =  Get File  /tmp/diagnose.log
    Should Not Contain  ${contents}  error  ignore_case=True
    Should Contain  ${contents}   Created tarfile: ${Files[0]} in directory ${TAR_FILE_DIRECTORY}


Diagnose Tool Gathers MDR Logs When Run From Installation
    [Tags]  DIAGNOSE  SMOKE  MDR_PLUGIN
    Wait Until Created  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcs_envelope.log     20 seconds

    Create Directory  ${TAR_FILE_DIRECTORY}

    Mimic MDR Files   ${SOPHOS_INSTALL}

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

    # Untar diagnose tar to check contents
    Create Directory  ${UNPACK_DIRECTORY}
    ${result} =   Run Process   tar    xzf    ${TAR_FILE_DIRECTORY}/${Files[0]}    -C    ${UNPACK_DIRECTORY}/
    Should Be Equal As Strings   ${result.rc}  0

    Check Diagnose Output For Additional MDR Plugin File
    Check Diagnose Output For System Command Files
    Check Diagnose Output For System Files

    ${contents} =  Get File  /tmp/diagnose.log
    Should Not Contain  ${contents}  error  ignore_case=True
    Should Contain  ${contents}   Created tarfile: ${Files[0]} in directory ${TAR_FILE_DIRECTORY}

Diagnose Tool Help Text
    ${result} =   Run Process   ${SOPHOS_INSTALL}/bin/sophos_diagnose  --help
    Should Be Equal   ${result.stderr}   Expected Usage: ./sophos_diagnose <path_to_output_directory>
    Should Be Equal As Integers   ${result.rc}  0

Diagnose Tool Bad Input Fails
    ${result} =   Run Process   ${SOPHOS_INSTALL}/bin/sophos_diagnose  asdasdasdasdasd
    Should Be Equal As Integers    ${result.rc}  3
    Should Contain   ${result.stderr}   Cause: No such file or directory
    Should Not Exist  /tmp/DiagnoseOutput

Diagnose Tool More Than Expected Input Fails
    ${result} =   Run Process   ${SOPHOS_INSTALL}/bin/sophos_diagnose  asdasdasdasdasd    dddddd
    Should Be Equal As Integers    ${result.rc}  1
    Should Be Equal   ${result.stderr}    	Expecting only one parameter got 2
    Should Not Exist  /tmp/DiagnoseOutput

Diagnose Tool No Input Creates Output Locally
    ${result} =   Run Process   ${SOPHOS_INSTALL}/bin/sophos_diagnose
    Log  ${result.stdout}
    Should Be Equal As Integers  ${result.rc}  0
    File Should Exist  sspl-diagnose_*.tar.gz
    Remove File  sspl-diagnose_*.tar.gz

Diagnose Tool Run Twice Creates Two Tar Files
    ${TAR_FILE_DIRECTORY} =  Set Variable  /tmp/TestOutputDirectory

    Create Directory  ${TAR_FILE_DIRECTORY}

    ${result} =   Run Process   ${SOPHOS_INSTALL}/bin/sophos_diagnose    ${TAR_FILE_DIRECTORY}
    Log  ${result.stdout}
    Should Be Equal As Integers  ${result.rc}  0
    # Check diagnose tar created
    ${Files} =  List Files In Directory  ${TAR_FILE_DIRECTORY}
    ${fileCount} =    Get length    ${Files}
    Should Be Equal As Numbers  ${fileCount}  1

    ${result} =   Run Process   ${SOPHOS_INSTALL}/bin/sophos_diagnose    ${TAR_FILE_DIRECTORY}

    # Check diagnose tar created
    ${Files} =  List Files In Directory  ${TAR_FILE_DIRECTORY}
    ${fileCount} =    Get length    ${Files}
    Should Be Equal As Numbers  ${fileCount}  2

    Should Contain    ${Files[0]}    sspl-diagnose_
    Should Contain    ${Files[0]}    .tar.gz
    Should Contain    ${Files[1]}    sspl-diagnose_
    Should Contain    ${Files[1]}    .tar.gz

Diagnose Tool Works If Files From Previous Run Of Tool Are Not Cleaned Up
    Remove Directory  ${TAR_FILE_DIRECTORY}  true
    Create Directory  ${TAR_FILE_DIRECTORY}
    Create Directory  ${TAR_FILE_DIRECTORY}/DiagnoseOutputaaaaa/DiagnoseOutput/PluginFiles
    ${result} =   Run Process   ${SOPHOS_INSTALL}/bin/sophos_diagnose  ${TAR_FILE_DIRECTORY}
    Should Be Equal As Integers    ${result.rc}  0
    Log    ${result.stderr}
    Should Not Contain   ${result.stderr}    	Error: Previous execution of Diagnose tool has not cleaned up. Please remove /tmp/DiagnoseOutput/PluginFiles

    File Should Exist  ${TAR_FILE_DIRECTORY}/sspl-diagnose_*.tar.gz

Diagnose Tool Deletes Temp Directory if tar fails
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
    Create Directory  ${TAR_FILE_DIRECTORY}
    Run  chmod 700 ${TAR_FILE_DIRECTORY}
    ${retcode} =  Run Diagnose    ${SOPHOS_INSTALL}/bin/     ${TAR_FILE_DIRECTORY}
    LogUtils.Dump Log    ${TAR_FILE_DIRECTORY}/diagnose.log
    Should Be Equal As Integers   ${retcode}  0

    ${permissions} =  Run Process  ls  -l    ${TAR_FILE_DIRECTORY}
    Log  ${permissions.stdout}
    ${result} =  Check File Permissions  ${TAR_FILE_DIRECTORY}
    Should Contain  ${result}   Correct Permissions

Diagnose Tool Does Not Gather JournalCtl Data Older Than 10 Days
    Create Directory  ${TAR_FILE_DIRECTORY}
    Run  chmod 700 ${TAR_FILE_DIRECTORY}
    ${result} =   Run Process   ${SOPHOS_INSTALL}/bin/sophos_diagnose    ${TAR_FILE_DIRECTORY}
    Log  ${result.stdout}
    Should Be Equal As Integers  ${result.rc}  0     msg=Diagnose Failed with message: ${result.stderr}

    # Untar diagnose tar to check contents
    ${Files} =  List Files In Directory  ${TAR_FILE_DIRECTORY}
    Create Directory  ${UNPACK_DIRECTORY}
    ${result} =   Run Process   tar    xzf    ${TAR_FILE_DIRECTORY}/${Files[0]}    -C    ${UNPACK_DIRECTORY}/
    Should Be Equal As Strings   ${result.rc}  0

    Check Journalclt Files Do Not Have Old Entries   /tmp/DiagnoseOutput/SystemFiles    days=11

Diagnose Tool Fails Due To Full Disk Partition And Should Not Generate Uncaught Exception
    [Tags]  DIAGNOSE  SMOKE
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
    Should Contain   ${result.stderr}   contents failed to copy. Check space available on device.

Diagnose Tool Fails Due To Read Only Mount And Should Not Generate Uncaught Exception
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
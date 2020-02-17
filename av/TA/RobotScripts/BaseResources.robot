*** Settings ***

Library         ../Libs/BaseInteractionTools/DiagnoseUtils.py
Library         String

*** Variables ***

${TAR_FILE_DIRECTORY} =  /tmp/TestOutputDirectory
${UNPACK_DIRECTORY} =  /tmp/DiagnoseOutput

*** Keywords ***

Run Diagnose
    Create Directory  ${TAR_FILE_DIRECTORY}
    ${retcode} =  Start Diagnose  ${SOPHOS_INSTALL}/bin/sophos_diagnose  ${TAR_FILE_DIRECTORY}
#    ${content} =  Get File   /tmp/diagnose.log
#    Should Contain  ${content}  absolutenonsense
    Should Be Equal As Integers   ${retcode}  0

Check Diagnose Tar Created
    ${Files} =  List Files In Directory  ${TAR_FILE_DIRECTORY}/
    ${fileCount} =    Get length    ${Files}
    Should Be Equal As Numbers  ${fileCount}  1
    Should Contain    ${Files[0]}    sspl-diagnose
    Should Not Contain   ${Files}  BaseFiles
    Should Not Contain   ${Files}  SystemFiles
    Should Not Contain   ${Files}  PluginFiles

Check Diagnose Collects Correct AV Files
    ${Files} =  List Files In Directory  ${TAR_FILE_DIRECTORY}/
    Create Directory  ${UNPACK_DIRECTORY}
    ${result} =   Run Process   tar    xzf    ${TAR_FILE_DIRECTORY}/${Files[0]}    -C    ${UNPACK_DIRECTORY}/
    Should Be Equal As Strings   ${result.rc}  0

    ${PluginFiles} =  List Files In Directory  /tmp/DiagnoseOutput/PluginFiles
    ${AVFiles} =  List Files In Directory  /tmp/DiagnoseOutput/PluginFiles/av

    Should Contain  ${PluginFiles}  av.log
    Should Contain  ${PluginFiles}  sophos_threat_detector.log

    Should Contain  ${AVFiles}  VERSION.ini
    Should Contain  ${AVFiles}  VERSION.ini.0


Check Diagnose Logs
    ${Files} =  List Files In Directory  ${TAR_FILE_DIRECTORY}/
    ${contents} =  Get File  /tmp/diagnose.log
    Should Not Contain  ${contents}  error  ignore_case=True
    Should Contain  ${contents}   Created tarfile: ${Files[0]} in directory ${TAR_FILE_DIRECTORY}
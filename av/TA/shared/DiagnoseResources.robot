*** Settings ***

Library         ../Libs/BaseInteractionTools/PolicyUtils.py
Library         ../Libs/OnFail.py
Library         String
Library         DateTime

*** Variables ***

${TAR_FILE_DIRECTORY} =  /tmp/TestOutputDirectory
${UNPACK_DIRECTORY} =  /tmp/DiagnoseOutput

*** Keywords ***

Run Diagnose
    Register Cleanup   Remove Directory  ${TAR_FILE_DIRECTORY}  recursive=True
    Create Directory  ${TAR_FILE_DIRECTORY}
    Empty Directory  ${TAR_FILE_DIRECTORY}
    Directory Should Be Empty  ${TAR_FILE_DIRECTORY}
    ${retcode} =  Run And Return Rc  ${SOPHOS_INSTALL}/bin/sophos_diagnose ${TAR_FILE_DIRECTORY}
    Should Be Equal As Integers   ${retcode}  0

Check Diagnose Tar Created
    ${Files} =  List Files In Directory  ${TAR_FILE_DIRECTORY}/
    ${fileCount} =    Get length    ${Files}
    Should Be Equal As Integers  ${fileCount}  1
    Should Contain    ${Files[0]}    sspl-diagnose
    Should Not Contain   ${Files}  BaseFiles
    Should Not Contain   ${Files}  SystemFiles
    Should Not Contain   ${Files}  PluginFiles

Check Diagnose Collects Correct AV Files
    ${Files} =  List Files In Directory  ${TAR_FILE_DIRECTORY}/
    Register Cleanup   Remove Directory  ${UNPACK_DIRECTORY}  recursive=True
    Create Directory  ${UNPACK_DIRECTORY}
    Empty Directory  ${UNPACK_DIRECTORY}
    ${result} =   Run Process   tar    xvzf    ${TAR_FILE_DIRECTORY}/${Files[0]}    -C    ${UNPACK_DIRECTORY}/
    Should Be Equal As Integers  ${result.rc}  0
    Log  ${result.stdout}

    ${TopLevelDirs} =  List Directories In Directory  ${UNPACK_DIRECTORY}
    ${AVPFiles} =  List Files In Directory  ${UNPACK_DIRECTORY}/${TopLevelDirs[0]}/PluginFiles/av
    ${SophosThreatDetectorFiles} =  List Files In Directory  ${UNPACK_DIRECTORY}/${TopLevelDirs[0]}/PluginFiles/av/chroot/log

    Should Contain  ${AVPFiles}  av.log
    Should Contain  ${AVPFiles}  Scan Now.log
    Should Contain  ${SophosThreatDetectorFiles}  sophos_threat_detector.log
    Should Contain  ${SophosThreatDetectorFiles}  susi_debug.log

    Should Contain  ${AVPFiles}  VERSION.ini
    Should Contain  ${AVPFiles}  VERSION.ini.0


Check Diagnose Logs
    ${TopLevelDirs} =  List Directories In Directory  ${UNPACK_DIRECTORY}
    ${contents} =  Get File  ${UNPACK_DIRECTORY}/${TopLevelDirs[0]}/BaseFiles/diagnose.log
    Should Contain  ${contents}  Completed gathering files
    Should Not Contain  ${contents}  ERROR


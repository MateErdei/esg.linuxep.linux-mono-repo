*** Settings ***

Library         ../Libs/BaseInteractionTools/PolicyUtils.py
Library         ${COMMON_TEST_LIBS}/OnFail.py
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
    Should Be Equal As Integers   ${retcode}  ${0}

Get DiagnoseOutput
    ${Files} =  List Files In Directory  ${TAR_FILE_DIRECTORY}/
    Register Cleanup   Remove Directory  ${UNPACK_DIRECTORY}  recursive=True
    Create Directory  ${UNPACK_DIRECTORY}
    Empty Directory  ${UNPACK_DIRECTORY}
    ${result} =   Run Process   tar    xvzf    ${TAR_FILE_DIRECTORY}/${Files[0]}    -C    ${UNPACK_DIRECTORY}/
    Should Be Equal As Integers  ${result.rc}  ${0}
    Log  ${result.stdout}
    ${folder}=  Fetch From Left   ${Files[0]}   .tar.gz
    Set Test Variable  ${DiagnoseOutput}  ${folder}


Run Diagnose And Get DiagnoseOutput
    Run Diagnose
    Get DiagnoseOutput


Check Diagnose Tar Created
    ${Files} =  List Files In Directory  ${TAR_FILE_DIRECTORY}/
    ${fileCount} =    Get length    ${Files}
    Should Be Equal As Integers  ${fileCount}  ${1}
    Should Contain    ${Files[0]}    sspl-diagnose
    Should Not Contain   ${Files}  BaseFiles
    Should Not Contain   ${Files}  SystemFiles
    Should Not Contain   ${Files}  PluginFiles


Check Diagnose Collects Correct AV Files
    ${AVPFiles} =  List Files In Directory  ${UNPACK_DIRECTORY}/${DiagnoseOutput}/PluginFiles/av
    ${SophosThreatDetectorFiles} =  List Files In Directory  ${UNPACK_DIRECTORY}/${DiagnoseOutput}/PluginFiles/av/chroot/log

    Should Contain  ${AVPFiles}  av.log
    Should Contain  ${AVPFiles}  Scan Now.log
    Should Contain  ${SophosThreatDetectorFiles}  sophos_threat_detector.log
    Should Contain  ${SophosThreatDetectorFiles}  susi_debug.log

    Should Contain  ${AVPFiles}  VERSION.ini
    Should Contain  ${AVPFiles}  VERSION.ini.0


Check Diagnose Contains File In TD Var
    [arguments]    ${file}
    ${SophosThreatDetectorFiles} =  List Files In Directory  ${UNPACK_DIRECTORY}/${DiagnoseOutput}/PluginFiles/av/chroot/var
    Should Contain  ${SophosThreatDetectorFiles}  ${file}


Check Diagnose Contains File In AV Var
    [arguments]    ${file}
    ${AVFiles} =  List Files In Directory  ${UNPACK_DIRECTORY}/${DiagnoseOutput}/PluginFiles/av/var
    Should Contain  ${AVFiles}  ${file}


Check Diagnose Logs
    ${contents} =  Get File  ${UNPACK_DIRECTORY}/${DiagnoseOutput}/BaseFiles/diagnose.log
    Should Contain  ${contents}  Completed gathering files
    Should Not Contain  ${contents}  ERROR
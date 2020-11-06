*** Settings ***
Documentation   Product tests for SSPL-AV Command line scanner
Force Tags      PRODUCT  AVSCANNER
Library         Process
Library         Collections
Library         OperatingSystem
Library         ../Libs/FakeManagement.py
Library         ../Libs/AVScanner.py
Library         ../Libs/OnFail.py

Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVResources.robot

Suite Setup     AVCommandLineScanner Suite Setup
Suite Teardown  AVCommandLineScanner Suite TearDown

Test Setup      AVCommandLineScanner Test Setup
Test Teardown   AVCommandLineScanner Test TearDown


*** Keywords ***

AVCommandLineScanner Suite Setup
    Run Keyword And Ignore Error   Empty Directory   ${COMPONENT_ROOT_PATH}/log
    Run Keyword And Ignore Error   Empty Directory   ${SOPHOS_INSTALL}/tmp
    Start Fake Management
    Start AV

AVCommandLineScanner Suite TearDown
    Stop AV
    Stop Fake Management
    Terminate All Processes  kill=True

Reset AVCommandLineScanner Suite
    AVCommandLineScanner Suite TearDown
    AVCommandLineScanner Suite Setup

AVCommandLineScanner Test Setup
    Log  AVCommandLineScanner Test Setup

AVCommandLineScanner Test TearDown
    run teardown functions
    Run Keyword If Test Failed  Run Keyword And Ignore Error  Log File   ${COMPONENT_ROOT_PATH}/log/${COMPONENT_NAME}.log  encoding_errors=replace
    Run Keyword If Test Failed  Run Keyword And Ignore Error  Log File   ${FAKEMANAGEMENT_AGENT_LOG_PATH}  encoding_errors=replace
    Run Keyword If Test Failed  Run Keyword And Ignore Error  Log File   ${THREAT_DETECTOR_LOG_PATH}  encoding_errors=replace
    Run Keyword If Test Failed  Reset AVCommandLineScanner Suite

Clear threat detector log
    Remove File   ${THREAT_DETECTOR_LOG_PATH}

Clear logs
    Clear threat detector log

Start AV
    Clear logs
    ${handle} =  Start Process  ${AV_PLUGIN_BIN}
    Set Suite Variable  ${AV_PLUGIN_HANDLE}  ${handle}
    Check AV Plugin Installed
    Wait until threat detector running

Stop AV
     ${result} =  Terminate Process  ${AV_PLUGIN_HANDLE}

*** Variables ***
${CLI_SCANNER_PATH}  ${COMPONENT_ROOT_PATH}/bin/avscanner
${CLEAN_STRING}     not an eicar
${NORMAL_DIRECTORY}     /home/vagrant/this/is/a/directory/for/scanning
${LONG_DIRECTORY}   0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
${CLEAN_RESULT}     0
${VIRUS_DETECTED_RESULT}     ${69}
${UKNOWN_OPTION_RESULT}      2
${FILE_NOT_FOUND_RESULT}     2
${PERMISSION_DENIED_RESULT}  13
${BAD_OPTION_RESULT}         3
${CUSTOM_OUTPUT_FILE}   /home/vagrant/output
${PERMISSIONS_TEST}     ${NORMAL_DIRECTORY}/permissions_test
*** Test Cases ***

CLS No args
    ${result} =  Run Process  ${CLI_SCANNER_PATH}  timeout=120s  stderr=STDOUT

    Log To Console  return code is ${result.rc}
    Log To Console  output is ${result.stdout}

    Should Not Contain  ${result.stdout.replace("\n", " ")}  "failed to execute"


CLS Can Scan Clean File
    Create File     ${NORMAL_DIRECTORY}/clean_file    ${CLEAN_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/clean_file

    Log To Console  return code is ${rc}
    Log To Console  output is ${output}
    Should Not Contain  ${output}  Scanning of ${NORMAL_DIRECTORY}/clean_file was aborted
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}


CLS Does Not Ordinarily Output To Stderr
    Create File     ${NORMAL_DIRECTORY}/clean_file    ${CLEAN_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/clean_file 1>/dev/null

    Log To Console  return code is ${rc}
    Log To Console  output is ${output}
    Should Be Empty  ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}


CLS Can Scan Infected File
   Create File     ${NORMAL_DIRECTORY}/naugthy_eicar    ${EICAR_STRING}
   ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/naugthy_eicar

   Log To Console  return code is ${rc}
   Log To Console  output is ${output}
   Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
   File Log Contains   ${THREAT_DETECTOR_LOG_PATH}   Detected "EICAR-AV-Test" in ${NORMAL_DIRECTORY}/naugthy_eicar


CLS Can Scan Shallow Archive But not Deep Archive
    Create File     ${NORMAL_DIRECTORY}/eicar    ${EICAR_STRING}
    create archive test files  ${NORMAL_DIRECTORY}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/eicar5.zip -s
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/eicar6.zip -s
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/eicar15.tar -s
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/eicar16.tar -s
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}

CLS Summary is Correct
   Create File     ${NORMAL_DIRECTORY}/naugthy_eicar    ${EICAR_STRING}
   Create File     ${NORMAL_DIRECTORY}/clean_file    ${CLEAN_STRING}
   ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/naugthy_eicar ${NORMAL_DIRECTORY}/clean_file

   Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
   Should Contain   ${output}  2 files scanned in
   Should Contain   ${output}  1 file out of 2 was infected.
   Should Contain   ${output}  1 EICAR-AV-Test infection discovered.


CLS Does not request TFTClassification from SUSI
      Create File     ${NORMAL_DIRECTORY}/naugthy_eicar    ${EICAR_STRING}
      ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/naugthy_eicar

      Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
      File Log Contains   ${THREAT_DETECTOR_LOG_PATH}   Detected "EICAR-AV-Test" in ${NORMAL_DIRECTORY}/naugthy_eicar
      Should Not Contain  ${THREAT_DETECTOR_LOG_PATH}  TFTClassifications

CLS Can Scan Archive File
      ${ARCHIVE_DIR} =  Set Variable  ${NORMAL_DIRECTORY}/archive_dir
      Create Directory  ${ARCHIVE_DIR}
      Create File  ${ARCHIVE_DIR}/1_eicar    ${EICAR_STRING}
      Create File  ${ARCHIVE_DIR}/2_clean    ${CLEAN_STRING}
      Create File  ${ARCHIVE_DIR}/3_eicar    ${EICAR_STRING}
      Create File  ${ARCHIVE_DIR}/4_clean    ${CLEAN_STRING}
      Create File  ${ARCHIVE_DIR}/5_eicar    ${EICAR_STRING}

      Run Process     tar  -cf  ${NORMAL_DIRECTORY}/test.tar  ${ARCHIVE_DIR}
      ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/test.tar --scan-archives

      Log To Console  return code is ${rc}
      Log To Console  output is ${output}
      Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
      Should Contain  ${output}  Detected "${NORMAL_DIRECTORY}/test.tar${ARCHIVE_DIR}/1_eicar" is infected with EICAR-AV-Test
      Should Contain  ${output}  Detected "${NORMAL_DIRECTORY}/test.tar${ARCHIVE_DIR}/3_eicar" is infected with EICAR-AV-Test
      Should Contain  ${output}  Detected "${NORMAL_DIRECTORY}/test.tar${ARCHIVE_DIR}/5_eicar" is infected with EICAR-AV-Test

CLS Doesnt Detect eicar in zip without archive option
      ${ARCHIVE_DIR} =  Set Variable  ${NORMAL_DIRECTORY}/archive_dir
      Create Directory  ${ARCHIVE_DIR}
      Create File  ${ARCHIVE_DIR}/1_eicar    ${EICAR_STRING}

      Run Process     zip  ${NORMAL_DIRECTORY}/test.zip  ${ARCHIVE_DIR}
      ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/test.zip

      Log  return code is ${rc}
      Log  output is ${output}
      Should Be Equal As Integers  ${rc}  ${0}
      Should Not Contain  ${output}  Detected "${NORMAL_DIRECTORY}/test.zip${ARCHIVE_DIR}/1_eicar" is infected with EICAR-AV-Test
      Should Not Contain  ${output}  is infected with EICAR-AV-Test

CLS Can Scan Multiple Archive Files
      ${ARCHIVE_DIR} =  Set Variable  ${NORMAL_DIRECTORY}/archive_dir
      ${SCAN_DIR} =  Set Variable  ${NORMAL_DIRECTORY}/scan_dir

      Create Directory  ${ARCHIVE_DIR}
      Create Directory  ${SCAN_DIR}

      Create File  ${ARCHIVE_DIR}/1_eicar    ${EICAR_STRING}
      Create File  ${ARCHIVE_DIR}/2_clean    ${CLEAN_STRING}
      Create File  ${ARCHIVE_DIR}/3_eicar    ${EICAR_STRING}
      Create File  ${ARCHIVE_DIR}/4_clean    ${CLEAN_STRING}
      Create File  ${ARCHIVE_DIR}/5_eicar    ${EICAR_STRING}

      Run Process     tar  -cf  ${SCAN_DIR}/test.tar  ${ARCHIVE_DIR}
      Run Process     tar  -czf  ${SCAN_DIR}/test.tgz  ${ARCHIVE_DIR}
      Run Process     tar  -cjf  ${SCAN_DIR}/test.tar.bz2  ${ARCHIVE_DIR}
      ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${SCAN_DIR} --scan-archives

      Log To Console  return code is ${rc}
      Log To Console  output is ${output}
      Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
      Should Contain  ${output}  Detected "${SCAN_DIR}/test.tar${ARCHIVE_DIR}/1_eicar" is infected with EICAR-AV-Test
      Should Contain  ${output}  Detected "${SCAN_DIR}/test.tar${ARCHIVE_DIR}/3_eicar" is infected with EICAR-AV-Test
      Should Contain  ${output}  Detected "${SCAN_DIR}/test.tar${ARCHIVE_DIR}/5_eicar" is infected with EICAR-AV-Test
      Should Contain  ${output}  Detected "${SCAN_DIR}/test.tgz/Gzip${ARCHIVE_DIR}/1_eicar" is infected with EICAR-AV-Test
      Should Contain  ${output}  Detected "${SCAN_DIR}/test.tgz/Gzip${ARCHIVE_DIR}/3_eicar" is infected with EICAR-AV-Test
      Should Contain  ${output}  Detected "${SCAN_DIR}/test.tgz/Gzip${ARCHIVE_DIR}/5_eicar" is infected with EICAR-AV-Test
      Should Contain  ${output}  Detected "${SCAN_DIR}/test.tar.bz2/Bzip2${ARCHIVE_DIR}/1_eicar" is infected with EICAR-AV-Test
      Should Contain  ${output}  Detected "${SCAN_DIR}/test.tar.bz2/Bzip2${ARCHIVE_DIR}/3_eicar" is infected with EICAR-AV-Test
      Should Contain  ${output}  Detected "${SCAN_DIR}/test.tar.bz2/Bzip2${ARCHIVE_DIR}/5_eicar" is infected with EICAR-AV-Test

      Remove Directory  ${SCAN_DIR}  recursive=True
      Remove Directory  ${ARCHIVE_DIR}  recursive=True

CLS Abort Scanning of Zip Bomb
      Copy File  ${RESOURCES_PATH}/file_samples/zipbomb.zip  ${NORMAL_DIRECTORY}

      ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/zipbomb.zip --scan-archives

      Log To Console  return code is ${rc}
      Log To Console  output is ${output}
      Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}
      Should Contain  ${output}  Scanning of ${NORMAL_DIRECTORY}/zipbomb.zip was aborted

AV Log Contains No Errors When Scanning File
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/naugthy_eicar

    Log To Console  return code is ${rc}
    Log To Console  output is ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

    Wait Until AV Plugin Log Contains  Sending threat detection notification to central

    AV Plugin Log Does Not Contain  ERROR

CLS Can Scan Infected And Clean File With The Same Name
    Create File     ${NORMAL_DIRECTORY}/naugthy_eicar_folder/eicar    ${EICAR_STRING}
    Create File     ${NORMAL_DIRECTORY}/clean_eicar_folder/eicar    ${CLEAN_STRING}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/naugthy_eicar_folder/eicar ${NORMAL_DIRECTORY}/clean_eicar_folder/eicar

    Log To Console  return code is ${rc}
    Log To Console  output is ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

    Log To Console  ${NORMAL_DIRECTORY}

CLS Does Not Detect PUAs
    Create File     ${NORMAL_DIRECTORY}/naugthy_eicar_folder/eicar_pua    ${EICAR_PUA_STRING}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/naugthy_eicar_folder/eicar_pua

    Log To Console  return code is ${rc}
    Log To Console  output is ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}

    Log To Console  ${NORMAL_DIRECTORY}


CLS Will Not Scan Non-Existent File
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/i_do_not_exist

    Log To Console  return code is ${rc}
    Log To Console  output is ${output}
    Should Be Equal As Integers  ${rc}  ${FILE_NOT_FOUND_RESULT}


CLS Will Not Scan Restricted File
    [Teardown]  Remove Directory  ${PERMISSIONS_TEST}  true

    Create Directory  ${PERMISSIONS_TEST}
    Create File     ${PERMISSIONS_TEST}/eicar    ${CLEAN_STRING}

    Run  chmod -x ${PERMISSIONS_TEST}

    ${command} =    Set Variable    ${CLI_SCANNER_PATH} ${PERMISSIONS_TEST}/eicar
    ${su_command} =    Set Variable    su -s /bin/sh -c "${command}" nobody
    ${rc}   ${output} =    Run And Return Rc And Output   ${su_command}

    Log To Console  return code is ${rc}
    Log To Console  output is ${output}
    Should Be Equal As Integers  ${rc}  ${PERMISSION_DENIED_RESULT}


CLS Will Not Scan Inside Restricted Folder
    [Teardown]  Remove Directory  ${PERMISSIONS_TEST}  true

    ${PERMISSIONS_TEST} =  Set Variable  ${NORMAL_DIRECTORY}/permissions_test

    Create Directory  ${PERMISSIONS_TEST}
    Create File     ${PERMISSIONS_TEST}/eicar    ${CLEAN_STRING}

    Run  chmod -x ${PERMISSIONS_TEST}

    ${command} =    Set Variable    ${CLI_SCANNER_PATH} ${PERMISSIONS_TEST}
    ${su_command} =    Set Variable    su -s /bin/sh -c "${command}" nobody
    ${rc}   ${output} =    Run And Return Rc And Output   ${su_command}

    Log To Console  return code is ${rc}
    Log To Console  output is ${output}

    Should Contain       ${output.replace("\n", " ")}  Failed to access
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}


CLS Can Scan Zero Byte File
     Create File  ${NORMAL_DIRECTORY}/zero_bytes
     ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/zero_bytes

     Log To Console  return code is ${rc}
     Log To Console  output is ${output}
     Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}

# Long Path is 4064 characters long
CLS Can Scan Long Path
    ${long_path} =  create long path  ${LONG_DIRECTORY}   ${40}  /home/vagrant/  clean_file
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${long_path}/clean_file

    Log To Console  return code is ${rc}
    Log To Console  output is ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}

# Huge Path is over 4064 characters long
CLS Cannot Scan Huge Path
    ${long_path} =  create long path  ${LONG_DIRECTORY}   ${100}  /home/vagrant/  clean_file
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${long_path}/clean_file

    Log To Console  return code is ${rc}
    Log To Console  output is ${output}
    Should Be Equal As Integers  ${rc}  36

# Huge Path is over 4064 characters long
CLS Can Scan Normal Path But Not SubFolders With a Huge Path
    ${long_path} =  create long path  ${LONG_DIRECTORY}   ${40}  /home/vagrant/  clean_file
    create long path  ${LONG_DIRECTORY}   ${100}  /home/vagrant/  clean_file
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${long_path}/

    Log To Console  return code is ${rc}
    Log To Console  output is ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}

CLS Creates Threat Report
   Create File     ${NORMAL_DIRECTORY}/naugthy_eicar    ${EICAR_STRING}
   ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/naugthy_eicar

   Log To Console  return code is ${rc}
   Log To Console  output is ${output}
   Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

   Wait Until AV Plugin Log Contains  Sending threat detection notification to central
   AV Plugin Log Contains  description="Found 'EICAR-AV-Test' in '${NORMAL_DIRECTORY}/naugthy_eicar'"
   AV Plugin Log Contains  type="sophos.mgt.msg.event.threat"
   AV Plugin Log Contains  domain="local"
   AV Plugin Log Contains  type="1"
   AV Plugin Log Contains  name="EICAR-AV-Test"
   AV Plugin Log Contains  scanType="203"
   AV Plugin Log Contains  status="200"
   AV Plugin Log Contains  id="Tfe8974b97b4b7a6a33b4c52acb4ffba0c11ebbf208a519245791ad32a96227d8"
   AV Plugin Log Contains  idSource="Tsha256(path,name)"
   AV Plugin Log Contains  <item file="naugthy_eicar"
   AV Plugin Log Contains  path="${NORMAL_DIRECTORY}/"/>
   AV Plugin Log Contains  <action action="101"/>

CLS Encoded Eicars
   ${result} =  Run Process  bash  ${BASH_SCRIPTS_PATH}/createEncodingEicars.sh
   Should Be Equal As Integers  ${result.rc}  0
   ${result} =  Run Process  ${CLI_SCANNER_PATH}  /tmp/encoded_eicars/  timeout=120s
   Should Be Equal As Integers  ${result.rc}  ${VIRUS_DETECTED_RESULT}

   # Once CORE-1517 has been fixed, uncomment the check below
   #Threat Detector Does Not Log Contain  Failed to parse response from SUSI
   AV Plugin Log Contains  Sending threat detection notification to central

   ${FILE_CONTENT}=    Get File  ${SUPPORT_FILES_PATH}/list_of_expected_encoded_eicars
   @{eicar_names_list}=    Split to lines  ${FILE_CONTENT}
   FOR  ${item}  IN  @{eicar_names_list}
        Wait Until AV Plugin Log Contains  ${item}
   END

   File Log Contains   ${THREAT_DETECTOR_LOG_PATH}  Detected "EICAR-AV-Test" in /tmp/encoded_eicars/NEWLINEDIR\\n/\\n/bin/sh
   File Log Contains   ${THREAT_DETECTOR_LOG_PATH}  Detected "EICAR-AV-Test" in /tmp/encoded_eicars/PairDoubleQuote-"VIRUS.com"
   File Log Contains   ${THREAT_DETECTOR_LOG_PATH}  Scan requested of /tmp/encoded_eicars/PairDoubleQuote-"VIRUS.com"
   File Log Contains   ${THREAT_DETECTOR_LOG_PATH}  Scan requested of /tmp/encoded_eicars/NEWLINEDIR\\n/\\n/bin/sh

   Remove Directory  /tmp/encoded_eicars  true

CLS Handles Wild Card Eicars
    Remove Directory     ${NORMAL_DIRECTORY}  recursive=True
    Create File     ${NORMAL_DIRECTORY}/*   ${CLEAN_STRING}
    Create File     ${NORMAL_DIRECTORY}/eicar.com    ${EICAR_STRING}

    ${rc}   ${output} =    Run And Return Rc And Output    cd ${NORMAL_DIRECTORY} && ${CLI_SCANNER_PATH} "*"

    Should Contain       ${output}  Scanning ${NORMAL_DIRECTORY}/*
    Should Not Contain   ${output}  Scanning ${NORMAL_DIRECTORY}/eicar.com
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}

    ${rc}   ${output} =    Run And Return Rc And Output   cd ${NORMAL_DIRECTORY} && ${CLI_SCANNER_PATH} *

    Should Contain      ${output}  Scanning ${NORMAL_DIRECTORY}/*
    Should Contain      ${output}  Scanning ${NORMAL_DIRECTORY}/eicar.com
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}


CLS Handles Eicar With The Same Name As An Option
    [Teardown]  Run Keywords    AVCommandLineScanner Test TearDown
    ...         AND             Remove File     /-x
    ...         AND             Remove File     /--exclude

    Remove Directory     ${NORMAL_DIRECTORY}  recursive=True
    Create File     ${NORMAL_DIRECTORY}/--exclude   ${EICAR_STRING}
    Create File     ${NORMAL_DIRECTORY}/-x   ${EICAR_STRING}
    Create File     /--exclude   ${EICAR_STRING}
    Create File     /-x   ${EICAR_STRING}


    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/--exclude
    Should Contain       ${output}  Scanning ${NORMAL_DIRECTORY}/--exclude
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/-x
    Should Contain       ${output}  Scanning ${NORMAL_DIRECTORY}/-x
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

    ${rc}   ${output} =    Run And Return Rc And Output  cd / && ${CLI_SCANNER_PATH} ./--exclude
    Should Contain       ${output}  Scanning /--exclude
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

    ${rc}   ${output} =    Run And Return Rc And Output  cd / && ${CLI_SCANNER_PATH} ./-x
    Should Contain       ${output}  Scanning /-x
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    ${rc}   ${output} =    Run And Return Rc And Output  cd / && ${CLI_SCANNER_PATH} -- -x
    Should Contain       ${output}  Scanning /-x
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

    ${rc}   ${output} =    Run And Return Rc And Output  cd / && ${CLI_SCANNER_PATH} -- --exclude
    Should Contain       ${output}  Scanning /--exclude
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}


CLS Exclusions Filename
    Remove Directory     ${NORMAL_DIRECTORY}  recursive=True
    Create File     ${NORMAL_DIRECTORY}/clean_eicar    ${CLEAN_STRING}
    Create File     ${NORMAL_DIRECTORY}/naugthy_eicar_folder/eicar    ${EICAR_STRING}
    Create File     ${NORMAL_DIRECTORY}/clean_eicar_folder/eicar    ${CLEAN_STRING}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY} --exclude eicar

    Log To Console  return code is ${rc}
    Log To Console  output is ${output}

    Should Contain       ${output}  Scanning ${NORMAL_DIRECTORY}/clean_eicar
    Should Contain       ${output}  Excluding file: ${NORMAL_DIRECTORY}/naugthy_eicar_folder/eicar
    Should Contain       ${output}  Excluding file: ${NORMAL_DIRECTORY}/clean_eicar_folder/eicar
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}

CLS Exclusions Folder
    Remove Directory     ${NORMAL_DIRECTORY}  recursive=True
    Create File     ${NORMAL_DIRECTORY}/clean_eicar    ${CLEAN_STRING}
    Create File     ${NORMAL_DIRECTORY}/naugthy_eicar_folder/eicar    ${EICAR_STRING}
    Create File     ${NORMAL_DIRECTORY}/clean_eicar_folder/eicar    ${CLEAN_STRING}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY} --exclude ${NORMAL_DIRECTORY}/

    Log To Console  return code is ${rc}
    Log To Console  output is ${output}

    Should Contain      ${output}  Excluding directory: ${NORMAL_DIRECTORY}/
    File Log Should Not Contain  ${AV_LOG_PATH}   Excluding file: ${NORMAL_DIRECTORY}/clean_eicar
    File Log Should Not Contain  ${AV_LOG_PATH}   Excluding file: ${NORMAL_DIRECTORY}/naugthy_eicar_folder/eicar
    File Log Should Not Contain  ${AV_LOG_PATH}   Excluding file: ${NORMAL_DIRECTORY}/clean_eicar_folder/eicar
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}

CLS Exclusions Folder And File
    Remove Directory     ${NORMAL_DIRECTORY}  recursive=True
    Create File     ${NORMAL_DIRECTORY}/clean_eicar    ${CLEAN_STRING}
    Create File     ${NORMAL_DIRECTORY}/naugthy_eicar_folder/eicar    ${EICAR_STRING}
    Create File     ${NORMAL_DIRECTORY}/clean_eicar_folder/eicar    ${CLEAN_STRING}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY} --exclude clean_eicar ${NORMAL_DIRECTORY}/clean_eicar_folder/

    Log To Console  return code is ${rc}
    Log To Console  output is ${output}

    Should Contain       ${output}  Excluding file: ${NORMAL_DIRECTORY}/clean_eicar
    Should Contain       ${output}  Scanning ${NORMAL_DIRECTORY}/naugthy_eicar_folder/eicar
    Should Contain       ${output}  Excluding directory: ${NORMAL_DIRECTORY}/clean_eicar_folder/
    File Log Should Not Contain  ${AV_LOG_PATH}   Excluding file: ${NORMAL_DIRECTORY}/clean_eicar
    File Log Should Not Contain  ${AV_LOG_PATH}   Excluding file: ${NORMAL_DIRECTORY}/clean_eicar_folder/eicar
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

CLS Can Change Log Level
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY} --log-level=WARN
    Should Contain       ${output}  Setting logger to log level: WARN

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}
    Should Contain       ${output}  Logger av configured for level: DEBUG

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY} --log-level=ERROR
    Should Contain       ${output}  Setting logger to log level: ERROR


CLS Prints Help and Failure When Options Are Spaced Incorrectly
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} --exclude= file
    Should Contain       ${output}   Failed to parse command line options: the argument for option '--exclude' should follow immediately after the equal sign
    Should Be Equal As Integers  ${rc}  ${BAD_OPTION_RESULT}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} --files= file
    Should Contain       ${output}   Failed to parse command line options: the argument for option '--files' should follow immediately after the equal sign
    Should Be Equal As Integers  ${rc}  ${BAD_OPTION_RESULT}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} --config= config
    Should Contain       ${output}   Failed to parse command line options: the argument for option '--config' should follow immediately after the equal sign
    Should Be Equal As Integers  ${rc}  ${BAD_OPTION_RESULT}


CLS Prints Help and Failure When Parsing Incomplete Arguments
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} --exclude
    Should Contain       ${output}   Failed to parse command line options: the required argument for option '--exclude' is missing
    Should Be Equal As Integers  ${rc}  ${BAD_OPTION_RESULT}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} --files
    Should Contain       ${output}   Failed to parse command line options: the required argument for option '--files' is missing
    Should Be Equal As Integers  ${rc}  ${BAD_OPTION_RESULT}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} --config
    Should Contain       ${output}   Failed to parse command line options: the required argument for option '--config' is missing
    Should Be Equal As Integers  ${rc}  ${BAD_OPTION_RESULT}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} -x
    Should Contain       ${output}   Failed to parse command line options: the required argument for option '--exclude' is missing
    Should Be Equal As Integers  ${rc}  ${BAD_OPTION_RESULT}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} -f
    Should Contain       ${output}   Failed to parse command line options: the required argument for option '--files' is missing
    Should Be Equal As Integers  ${rc}  ${BAD_OPTION_RESULT}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} -c
    Should Contain       ${output}   Failed to parse command line options: the required argument for option '--config' is missing
    Should Be Equal As Integers  ${rc}  ${BAD_OPTION_RESULT}


CLS Can Log To A File
   ${LOG_FILE}      Set Variable   ${NORMAL_DIRECTORY}/scan.log
   ${THREAT_FILE}   Set Variable   ${NORMAL_DIRECTORY}/eicar.com

   Create File     ${THREAT_FILE}    ${EICAR_STRING}
   ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${THREAT_FILE} --output ${LOG_FILE}

   Log To Console  return code is ${rc}
   Log To Console  output is ${output}
   Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

   File Log Contains    ${LOG_FILE}    "${THREAT_FILE}" is infected with EICAR-AV-Test

CLS Will Not Log To A Directory
   ${THREAT_FILE}   Set Variable   ${NORMAL_DIRECTORY}/eicar.com

   Create File     ${THREAT_FILE}    ${EICAR_STRING}
   ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${THREAT_FILE} --output ${NORMAL_DIRECTORY}

   Log To Console  return code is ${rc}
   Log To Console  output is ${output}
   Should Be Equal As Integers  ${rc}  ${BAD_OPTION_RESULT}

   Should Contain    ${output}    Failed to log to ${NORMAL_DIRECTORY} as it is a directory
   Should Contain    ${output}    Usage: avscanner PATH... [OPTION]...
   Should Not Contain    ${output}    "${THREAT_FILE}" is infected with EICAR-AV-Test


CLS Can Scan Infected File Via Symlink To Directory
   ${targetDir} =  Set Variable  ${NORMAL_DIRECTORY}/a/b
   ${sourceDir} =  Set Variable  ${NORMAL_DIRECTORY}/a/c
   Create Directory   ${targetDir}
   Create Directory   ${sourceDir}
   Create File     ${targetDir}/eicar.com    ${EICAR_STRING}
   Run Process   ln  -snf  ${targetDir}  ${sourceDir}/b
   ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${sourceDir}/b

   Log To Console  return code is ${rc}
   Log To Console  output is ${output}
   Should Contain       ${output.replace("\n", " ")}  Detected "${sourceDir}/b/eicar.com" (symlinked to ${targetDir}/eicar.com) is infected with EICAR-AV-Test
   Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

   File Log Contains   ${THREAT_DETECTOR_LOG_PATH}   Detected "EICAR-AV-Test" in ${sourceDir}/b/eicar.com


CLS Can Scan Infected File Via Symlink To File
   Create File     ${NORMAL_DIRECTORY}/eicar.com    ${EICAR_STRING}
   Run Process   ln  -snf  ${NORMAL_DIRECTORY}/eicar.com  ${NORMAL_DIRECTORY}/symlinkToEicar
   ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/symlinkToEicar

   Log To Console  return code is ${rc}
   Log To Console  output is ${output}
   Should Contain       ${output.replace("\n", " ")}  Detected "${NORMAL_DIRECTORY}/symlinkToEicar" (symlinked to ${NORMAL_DIRECTORY}/eicar.com) is infected with EICAR-AV-Test
   Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
   File Log Contains   ${THREAT_DETECTOR_LOG_PATH}   Detected "EICAR-AV-Test" in ${NORMAL_DIRECTORY}/symlinkToEicar


CLS Skips The Scanning Of Symlink Targets On Special Mount Points
   Run Process   ln  -snf  /proc/uptime  ${NORMAL_DIRECTORY}/symlinkToProcUptime
   ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/symlinkToProcUptime

   Log To Console  return code is ${rc}
   Log To Console  output is ${output}
   Should Contain       ${output.replace("\n", " ")}  Skipping the scanning of symlink target ("/proc/uptime") which is on excluded mount point: "/proc"
   Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}


CLS Reports Error Once When Using Custom Log File
    Remove File  ${CUSTOM_OUTPUT_FILE}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} "file_does_not_exist" --output ${CUSTOM_OUTPUT_FILE}
    ${content} =  Get File   ${CUSTOM_OUTPUT_FILE}  encoding_errors=replace
    ${lines} =  Get Lines Containing String     ${content}  file/folder does not exist

    ${count} =  Get Line Count   ${lines}
    Should Be Equal As Integers  ${2}  ${count}


CLS Scans root with non-canonical path
    ${exclusions} =  ExclusionHelper.Get Root Exclusions for avscanner except proc
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} /. -x ${exclusions}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} /./ -x ${exclusions}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} /boot/.. -x ${exclusions}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} /.. -x ${exclusions}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} / -x ${exclusions}

    File Log Should Not Contain     ${AV_LOG_PATH}      Scanning /proc/
    File Log Should Not Contain     ${AV_LOG_PATH}      Scanning /./proc/

CLS Scans Paths That Exist and Dont Exist
    Remove Directory     ${NORMAL_DIRECTORY}  recursive=True
    Create File     ${NORMAL_DIRECTORY}/clean_eicar    ${CLEAN_STRING}
    Create File     ${NORMAL_DIRECTORY}/.clean_eicar_folder/eicar    ${CLEAN_STRING}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/.clean_eicar_folder/eicar ${NORMAL_DIRECTORY}/.dont_exist/eicar ${NORMAL_DIRECTORY}/.doesnot_exist ${NORMAL_DIRECTORY}/clean_eicar

    Should Contain      ${output}  Scanning /home/vagrant/this/is/a/directory/for/scanning/.clean_eicar_folder/eicar
    Should Contain      ${output}  Failed to scan "/home/vagrant/this/is/a/directory/for/scanning/.dont_exist/eicar": file/folder does not exist
    Should Contain      ${output}  Failed to scan "/home/vagrant/this/is/a/directory/for/scanning/.doesnot_exist": file/folder does not exist
    Should Contain      ${output}  Scanning /home/vagrant/this/is/a/directory/for/scanning/clean_eicar

    Should Be Equal As Integers  ${rc}  ${FILE_NOT_FOUND_RESULT}

CLS Scans file on NFS
    [Tags]  NFS
    ${source} =       Set Variable  /tmp/nfsshare
    ${destination} =  Set Variable  /mnt/nfsshare

    Create Directory  ${source}
    Create File       ${source}/eicar.com    ${EICAR_STRING}
    Create Directory  ${destination}
    Create Local NFS Share   ${source}   ${destination}
    register cleanup    Remove Local NFS Share   ${source}   ${destination}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${destination}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

CLS Reconnects And Continues Scan If Sophos Threat Detector Is Restarted
   ${LOG_FILE} =          Set Variable   ${NORMAL_DIRECTORY}/scan.log

   ${HANDLE} =    Start Process    ${CLI_SCANNER_PATH}   /   stdout=${LOG_FILE}   stderr=STDOUT
   Wait Until Keyword Succeeds
   ...  60 secs
   ...  1 secs
   ...  File Log Contains  ${LOG_FILE}  Scanning
   Stop AV
   Start AV
   Wait Until Keyword Succeeds
   ...  120 secs
   ...  1 secs
   ...  File Log Contains  ${LOG_FILE}  Reconnected to Sophos Threat Detector
   File Log Should Not Contain  ${LOG_FILE}  Reached total maximum number of reconnection attempts. Aborting scan.

   ${offset} =  Count File Log Lines   ${LOG_FILE}
   Wait Until Keyword Succeeds
   ...  60 secs
   ...  1 secs
   ...  File Log Contains With Offset  ${LOG_FILE}   Scanning   offset=${offset}

   Terminate Process   handle=${HANDLE}

CLS Aborts Scan If Sophos Threat Detector Is Killed And Does Not Recover
   ${LOG_FILE} =          Set Variable   ${NORMAL_DIRECTORY}/scan.log
   ${DETECTOR_BINARY} =   Set Variable   ${SOPHOS_INSTALL}/plugins/${COMPONENT}/sbin/sophos_threat_detector_launcher

   ${HANDLE} =    Start Process    ${CLI_SCANNER_PATH}   /   stdout=${LOG_FILE}   stderr=STDOUT
   # Rename the sophos threat detector launcher so that it cannot be restarted
   Move File  ${DETECTOR_BINARY}  ${DETECTOR_BINARY}_moved
   register cleanup  Move File  ${DETECTOR_BINARY}_moved  ${DETECTOR_BINARY}

   Wait Until Keyword Succeeds
   ...  60 secs
   ...  5 secs
   ...  File Log Contains  ${LOG_FILE}  Scanning
   ${rc}   ${output} =    Run And Return Rc And Output    pgrep sophos_threat
   Run Process   /bin/kill   -SIGSEGV   ${output}
   sleep  60  Waiting for the socket to timeout
   Wait Until Keyword Succeeds
   ...  240 secs
   ...  10 secs
   ...  File Log Contains  ${LOG_FILE}  Reached total maximum number of reconnection attempts. Aborting scan.

   Wait For Process   handle=${HANDLE}

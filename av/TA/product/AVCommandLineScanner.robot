*** Settings ***
Documentation   Product tests for SSPL-AV Command line scanner
Default Tags    PRODUCT  AVSCANNER
Library         Process
Library         Collections
Library         OperatingSystem
Library         ../Libs/FakeManagement.py
Library         ../Libs/AVScanner.py

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
${VIRUS_DETECTED_RESULT}    69
${UKNOWN_OPTION_RESULT}     2
${BAD_OPTION_RESULT}        3
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
   File Log Contains   ${THREAT_DETECTOR_LOG_PATH}   Detected "EICAR-AV-Test" in "${NORMAL_DIRECTORY}/naugthy_eicar"

CLS Can Scan Archive File
      Create File     ${NORMAL_DIRECTORY}/naugthy_eicar    ${EICAR_STRING}
      Run Process     tar  -cf  ${NORMAL_DIRECTORY}/naugthy_eicar.tar  ${NORMAL_DIRECTORY}/naugthy_eicar
      ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/naugthy_eicar.tar --scan-archives

      Log To Console  return code is ${rc}
      Log To Console  output is ${output}
      Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}


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


CLS Will Not Scan Non-Existent File
   ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/i_do_not_exist

   Log To Console  return code is ${rc}
   Log To Console  output is ${output}
   Should Be Equal As Integers  ${rc}  2

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

   Remove Directory  /tmp/encoded_eicars  true

CLS Exclusions Filename
   Remove Directory     ${NORMAL_DIRECTORY}  recursive=True
   Create File     ${NORMAL_DIRECTORY}/clean_eicar    ${CLEAN_STRING}
   Create File     ${NORMAL_DIRECTORY}/naugthy_eicar_folder/eicar    ${EICAR_STRING}
   Create File     ${NORMAL_DIRECTORY}/clean_eicar_folder/eicar    ${CLEAN_STRING}

   ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY} --exclude eicar

   Log To Console  return code is ${rc}
   Log To Console  output is ${output}

   Should Contain       ${output.replace("\n", " ")}  Scanning ${NORMAL_DIRECTORY}/clean_eicar
   Should Contain       ${output.replace("\n", " ")}  Excluding "${NORMAL_DIRECTORY}/naugthy_eicar_folder/eicar"
   Should Contain       ${output.replace("\n", " ")}  Excluding "${NORMAL_DIRECTORY}/clean_eicar_folder/eicar"
   Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}

CLS Exclusions Folder
   sleep  5m
   Remove Directory     ${NORMAL_DIRECTORY}  recursive=True
   Create File     ${NORMAL_DIRECTORY}/clean_eicar    ${CLEAN_STRING}
   Create File     ${NORMAL_DIRECTORY}/naugthy_eicar_folder/eicar    ${EICAR_STRING}
   Create File     ${NORMAL_DIRECTORY}/clean_eicar_folder/eicar    ${CLEAN_STRING}

   ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY} --exclude ${NORMAL_DIRECTORY}/

   Log To Console  return code is ${rc}
   Log To Console  output is ${output}

   Should Contain      ${output.replace("\n", " ")}  Excluding "${NORMAL_DIRECTORY}/clean_eicar"
   Should Contain      ${output.replace("\n", " ")}  Excluding "${NORMAL_DIRECTORY}/naugthy_eicar_folder"
   Should Contain      ${output.replace("\n", " ")}  Excluding "${NORMAL_DIRECTORY}/clean_eicar_folder"
   Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}

CLS Exclusions Folder And File
   Remove Directory     ${NORMAL_DIRECTORY}  recursive=True
   Create File     ${NORMAL_DIRECTORY}/clean_eicar    ${CLEAN_STRING}
   Create File     ${NORMAL_DIRECTORY}/naugthy_eicar_folder/eicar    ${EICAR_STRING}
   Create File     ${NORMAL_DIRECTORY}/clean_eicar_folder/eicar    ${CLEAN_STRING}

   ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY} --exclude clean_eicar ${NORMAL_DIRECTORY}/clean_eicar_folder/

   Log To Console  return code is ${rc}
   Log To Console  output is ${output}

   Should Contain       ${output.replace("\n", " ")}  Excluding "${NORMAL_DIRECTORY}/clean_eicar"
   Should Contain       ${output.replace("\n", " ")}  Scanning ${NORMAL_DIRECTORY}/naugthy_eicar_folder/eicar
   Should Contain       ${output.replace("\n", " ")}  Excluding "${NORMAL_DIRECTORY}/clean_eicar_folder/eicar"
   Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

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
   File Log Contains   ${THREAT_DETECTOR_LOG_PATH}   Detected "EICAR-AV-Test" in "${sourceDir}/b/eicar.com"


CLS Can Scan Infected File Via Symlink To File
   Create File     ${NORMAL_DIRECTORY}/eicar.com    ${EICAR_STRING}
   Run Process   ln  -snf  ${NORMAL_DIRECTORY}/eicar.com  ${NORMAL_DIRECTORY}/symlinkToEicar
   ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/symlinkToEicar

   Log To Console  return code is ${rc}
   Log To Console  output is ${output}
   Should Contain       ${output.replace("\n", " ")}  Detected "${NORMAL_DIRECTORY}/symlinkToEicar" (symlinked to ${NORMAL_DIRECTORY}/eicar.com) is infected with EICAR-AV-Test
   Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
   File Log Contains   ${THREAT_DETECTOR_LOG_PATH}   Detected "EICAR-AV-Test" in "${NORMAL_DIRECTORY}/symlinkToEicar"

CLS Scans root with non-canonical path
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} /. -x /boot/ /dev/ /etc/ /home/ /media/ /mnt/ /opt/ /root/ /run/ /srv/ /tmp/ /usr/ /var/ /vagrant/ /lib/ /lib32/ /bin/ /uk-filer5/prodro/ /sbin/
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} /./ -x /boot/ /dev/ /etc/ /home/ /media/ /mnt/ /opt/ /root/ /run/ /srv/ /tmp/ /usr/ /var/ /vagrant/ /lib/ /lib32/ /bin/ /uk-filer5/prodro/ /sbin/
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} /boot/.. -x /boot/ /dev/ /etc/ /home/ /media/ /mnt/ /opt/ /root/ /run/ /srv/ /tmp/ /usr/ /var/ /vagrant/ /lib/ /lib32/ /bin/ /uk-filer5/prodro/ /sbin/
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} /.. -x /boot/ /dev/ /etc/ /home/ /media/ /mnt/ /opt/ /root/ /run/ /srv/ /tmp/ /usr/ /var/ /vagrant/ /lib/ /lib32/ /bin/ /uk-filer5/prodro/ /sbin/
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} / -x /boot/ /dev/ /etc/ /home/ /media/ /mnt/ /opt/ /root/ /run/ /srv/ /tmp/ /usr/ /var/ /vagrant/ /lib/ /lib32/ /bin/ /uk-filer5/prodro/ /sbin/

    File Log Should Not Contain     ${AV_LOG_PATH}      Scanning /proc/
    File Log Should Not Contain     ${AV_LOG_PATH}      Scanning /./proc/

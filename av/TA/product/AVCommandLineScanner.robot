*** Settings ***
Documentation   Product tests for AVP Command line scanner
Default Tags    PRODUCT
Library         Process
Library         Collections
Library         OperatingSystem
Library         ../Libs/FakeManagement.py
Library         ../Libs/AVScanner.py

Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVResources.robot

*** Keywords ***

Start AV
    ${handle} =  Start Process  ${AV_PLUGIN_BIN}
    Set Test Variable  ${AV_PLUGIN_HANDLE}  ${handle}
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
CLS Can Scan Clean File
    Start AV

    Create File     ${NORMAL_DIRECTORY}/clean_eicar    ${CLEAN_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/clean_eicar

    Log To Console  return code is ${rc}
    Log To Console  output is ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}

    Stop AV

CLS Can Scan Infected File
   Start AV

   Create File     ${NORMAL_DIRECTORY}/naugthy_eicar    ${EICAR_STRING}
   ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/naugthy_eicar

   Log To Console  return code is ${rc}
   Log To Console  output is ${output}
   Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

   Stop AV

CLS Can Scan Archive File
      Start AV

      Create File     ${NORMAL_DIRECTORY}/naugthy_eicar    ${EICAR_STRING}
      Run Process     tar  -cf  ${NORMAL_DIRECTORY}/naugthy_eicar.tar  ${NORMAL_DIRECTORY}/naugthy_eicar
      ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/naugthy_eicar.tar --scan-archives

      Log To Console  return code is ${rc}
      Log To Console  output is ${output}
      Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

      Stop AV


AV Log Contains No Errors When Scanning File
    Start AV

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/naugthy_eicar

    Log To Console  return code is ${rc}
    Log To Console  output is ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

    Wait Until AV Plugin Log Contains  Sending threat detection notification to central

    AV Plugin Log Does Not Contain  ERROR

    Stop AV

CLS Can Scan Infected And Clean File With The Same Name
   Start AV

   Create File     ${NORMAL_DIRECTORY}/naugthy_eicar_folder/eicar    ${EICAR_STRING}
   Create File     ${NORMAL_DIRECTORY}/clean_eicar_folder/eicar    ${CLEAN_STRING}

   ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/naugthy_eicar_folder/eicar ${NORMAL_DIRECTORY}/clean_eicar_folder/eicar

   Log To Console  return code is ${rc}
   Log To Console  output is ${output}
   Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

   Log To Console  ${NORMAL_DIRECTORY}

   Stop AV


CLS Will Not Scan Non-Existent File
   Start AV

   ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/i_do_not_exist

   Log To Console  return code is ${rc}
   Log To Console  output is ${output}
   Should Be Equal As Integers  ${rc}  2

   Stop AV

CLS Can Scan Zero Byte File
     Start AV

     Create File  ${NORMAL_DIRECTORY}/zero_bytes
     ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/zero_bytes

     Log To Console  return code is ${rc}
     Log To Console  output is ${output}
     Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}

     Stop AV

# Long Path is 4064 characters long
CLS Can Scan Long Path
    Start AV

    ${long_path} =  create long path  ${LONG_DIRECTORY}   ${40}  /home/vagrant/  clean_file
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${long_path}/clean_file

    Log To Console  return code is ${rc}
    Log To Console  output is ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}

    Stop AV

# Huge Path is over 4064 characters long
CLS Cannot Scan Huge Path
    Start AV

    ${long_path} =  create long path  ${LONG_DIRECTORY}   ${100}  /home/vagrant/  clean_file
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${long_path}/clean_file

    Log To Console  return code is ${rc}
    Log To Console  output is ${output}
    Should Be Equal As Integers  ${rc}  36

    Stop AV

# Huge Path is over 4064 characters long
CLS Can Scan Normal Path But Not SubFolders With a Huge Path
    Start AV

    ${long_path} =  create long path  ${LONG_DIRECTORY}   ${40}  /home/vagrant/  clean_file
    create long path  ${LONG_DIRECTORY}   ${100}  /home/vagrant/  clean_file
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${long_path}/

    Log To Console  return code is ${rc}
    Log To Console  output is ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}

    Stop AV

CLS Creates Threat Report
   Start AV

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

   Stop AV

CLS Encoded Eicars
   Start AV

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
        AV Plugin Log Contains  ${item}
   END

   Remove Directory  /tmp/encoded_eicars  true

   Stop AV

CLS Exclusions Filename
   Start AV

   Remove Directory     ${NORMAL_DIRECTORY}  recursive=True
   Create File     ${NORMAL_DIRECTORY}/clean_eicar    ${CLEAN_STRING}
   Create File     ${NORMAL_DIRECTORY}/naugthy_eicar_folder/eicar    ${EICAR_STRING}
   Create File     ${NORMAL_DIRECTORY}/clean_eicar_folder/eicar    ${CLEAN_STRING}

   ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY} --exclude eicar

   Log To Console  return code is ${rc}
   Log To Console  output is ${output}

   Should Contain       ${output.replace("\n", " ")}  Scanning ${NORMAL_DIRECTORY}/clean_eicar
   Should Contain       ${output.replace("\n", " ")}  Exclusion applied to: "${NORMAL_DIRECTORY}/naugthy_eicar_folder/eicar"
   Should Contain       ${output.replace("\n", " ")}  Exclusion applied to: "${NORMAL_DIRECTORY}/clean_eicar_folder/eicar"
   Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}

   Stop AV

CLS Exclusions Folder
   Start AV

   Remove Directory     ${NORMAL_DIRECTORY}  recursive=True
   Create File     ${NORMAL_DIRECTORY}/clean_eicar    ${CLEAN_STRING}
   Create File     ${NORMAL_DIRECTORY}/naugthy_eicar_folder/eicar    ${EICAR_STRING}
   Create File     ${NORMAL_DIRECTORY}/clean_eicar_folder/eicar    ${CLEAN_STRING}

   ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY} --exclude ${NORMAL_DIRECTORY}/

   Log To Console  return code is ${rc}
   Log To Console  output is ${output}

   Should Contain      ${output.replace("\n", " ")}  Exclusion applied to: "${NORMAL_DIRECTORY}/clean_eicar"
   Should Contain      ${output.replace("\n", " ")}  Exclusion applied to: "${NORMAL_DIRECTORY}/naugthy_eicar_folder"
   Should Contain      ${output.replace("\n", " ")}  Exclusion applied to: "${NORMAL_DIRECTORY}/clean_eicar_folder"
   Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}

   Stop AV

CLS Exclusions Folder And File
   Start AV

   Remove Directory     ${NORMAL_DIRECTORY}  recursive=True
   Create File     ${NORMAL_DIRECTORY}/clean_eicar    ${CLEAN_STRING}
   Create File     ${NORMAL_DIRECTORY}/naugthy_eicar_folder/eicar    ${EICAR_STRING}
   Create File     ${NORMAL_DIRECTORY}/clean_eicar_folder/eicar    ${CLEAN_STRING}

   ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY} --exclude clean_eicar ${NORMAL_DIRECTORY}/clean_eicar_folder/

   Log To Console  return code is ${rc}
   Log To Console  output is ${output}

   Should Contain       ${output.replace("\n", " ")}  Exclusion applied to: "${NORMAL_DIRECTORY}/clean_eicar"
   Should Contain       ${output.replace("\n", " ")}  Scanning ${NORMAL_DIRECTORY}/naugthy_eicar_folder/eicar
   Should Contain       ${output.replace("\n", " ")}  Exclusion applied to: "${NORMAL_DIRECTORY}/clean_eicar_folder/eicar"
   Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

   Stop AV

CLS Prints Help and Failure When Options Are Spaced Incorrectly
    Start AV

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
    Start AV

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

    Stop AV
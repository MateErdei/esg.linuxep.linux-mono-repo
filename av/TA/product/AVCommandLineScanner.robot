*** Settings ***
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

Stop AV
     ${result} =  Terminate Process  ${AV_PLUGIN_HANDLE}

*** Variables ***
${CLI_SCANNER_PATH}  ${COMPONENT_ROOT_PATH}/bin/avscanner
${CLEAN_STRING}     not an eicar
${NORMAL_DIRECTORY}     /home/vagrant/this/is/a/directory/for/scanning
${LONG_DIRECTORY}   0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000

*** Test Cases ***
CLS Can Scan Clean File
    Start AV

    Create File     ${NORMAL_DIRECTORY}/clean_eicar    ${CLEAN_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/clean_eicar

    Log To Console  return code is ${rc}
    Log To Console  output is ${output}
    Should Be Equal  ${rc}  ${0}

    Stop AV

CLS Can Scan Infected File
   Start AV

   Create File     ${NORMAL_DIRECTORY}/naugthy_eicar    ${EICAR_STRING}
   ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/naugthy_eicar

   Log To Console  return code is ${rc}
   Log To Console  output is ${output}
   Should Be Equal  ${rc}  ${69}

   Stop AV

CLS Can Scan Infected And Clean File With The Same Name
   Start AV

   Create File     ${NORMAL_DIRECTORY}/naugthy_eicar_folder/eicar    ${EICAR_STRING}
   Create File     ${NORMAL_DIRECTORY}/clean_eicar_folder/eicar    ${CLEAN_STRING}

   ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/naugthy_eicar_folder/eicar ${NORMAL_DIRECTORY}/clean_eicar_folder/eicar

   Log To Console  return code is ${rc}
   Log To Console  output is ${output}
   Should Be Equal  ${rc}  ${69}

   Log To Console  ${NORMAL_DIRECTORY}

   Stop AV


CLS Will Not Scan Non-Existent File
   Start AV

   ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/i_do_not_exist

   Log To Console  return code is ${rc}
   Log To Console  output is ${output}
   Should Be Equal  ${rc}  ${2}

   Stop AV

CLS Can Scan Zero Byte File
     Start AV

     Create File  ${NORMAL_DIRECTORY}/zero_bytes
     ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/zero_bytes

     Log To Console  return code is ${rc}
     Log To Console  output is ${output}
     Should Be Equal  ${rc}  ${0}

     Stop AV

# Long Path is 4064 characters long
CLS Can Scan Long Path
    Start AV

    ${long_path} =  create long path  ${LONG_DIRECTORY}   ${40}  /home/vagrant/  clean_file
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${long_path}/clean_file

    Log To Console  return code is ${rc}
    Log To Console  output is ${output}
    Should Be Equal  ${rc}  ${0}

    Stop AV

# Huge Path is over 4064 characters long
CLS Cannot Scan Huge Path
    Start AV

    ${long_path} =  create long path  ${LONG_DIRECTORY}   ${100}  /home/vagrant/  clean_file
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${long_path}/clean_file

    Log To Console  return code is ${rc}
    Log To Console  output is ${output}
    Should Be Equal  ${rc}  ${36}

    Stop AV

# Huge Path is over 4064 characters long
CLS Can Scan Normal Path But Not SubFolders With a Huge Path
    Start AV

    ${long_path} =  create long path  ${LONG_DIRECTORY}   ${40}  /home/vagrant/  clean_file
    create long path  ${LONG_DIRECTORY}   ${100}  /home/vagrant/  clean_file
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${long_path}/

    Log To Console  return code is ${rc}
    Log To Console  output is ${output}
    Should Be Equal  ${rc}  ${0}

    Stop AV

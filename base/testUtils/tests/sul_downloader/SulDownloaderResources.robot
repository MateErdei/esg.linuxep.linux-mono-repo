*** Settings ***
Library    Process
Library    OperatingSystem
Library    ${LIBS_DIRECTORY}/SulDownloader.py
Library    ${LIBS_DIRECTORY}/UpdateServer.py
Library    ${LIBS_DIRECTORY}/WarehouseGenerator.py

Resource  ../watchdog/LogControlResources.robot



*** Keywords ***
Install Package Via Warehouse
    [Arguments]   ${sdds_dir}  ${rigidname}  ${install_path}=${SOPHOS_INSTALL}
    #Set Environment Variable  HOME_FOLDER  ExamplePlugin

    Generate Update Certs
    Clear Warehouse Config

    Remove Directory  ${tmpdir}/TestInstallFiles/${rigidname}  recursive=${TRUE}
    Copy Directory     ${sdds_dir}  ${tmpdir}/TestInstallFiles/${rigidname}

    Add Component Warehouse Config   ${rigidname}   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/

    Generate Warehouse
    Start Update Server    1233    ${tmpdir}/temp_warehouse/customer_files/
    Start Update Server    1234    ${tmpdir}/temp_warehouse/warehouses/sophosmain/
    ${config} =    Create JSON Config    install_path=${install_path}  rigidname=${rigidname}
    Create File    ${tmpdir}/update_config.json    content=${config}
    Set Log Level For Component And Reset and Return Previous Log  suldownloader  DEBUG
    ${result} =    Run Process    ${SOPHOS_INSTALL}/base/bin/SulDownloader    ${tmpdir}/update_config.json    ${tmpdir}/output.json
    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"


Dump Suldownloader Log
    Should Exist   ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    ${contents} =   Get File  ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    Log   "suldownloader.log = ${contents}"
    Return from Keyword  ${contents}

Check Suldownloader Is Not Running
    ${result} =    Run Process  pgrep SulDownloader  shell=true
    Should Be Equal As Integers    ${result.rc}    1       msg="stdout:${result.stdout}\nerr: ${result.stderr}"


Setup Install SDDS3 Base
    Require Fresh Install
    Remove File    ${SOPHOS_INSTALL}/base/VERSION.ini.0
    Copy File  ${SYSTEMPRODUCT_TEST_INPUT}/sspl-base/VERSION.ini  ${SOPHOS_INSTALL}/base/VERSION.ini.0
    Remove File    ${SOPHOS_INSTALL}/base/update/rootcerts/rootca384.crt.0
    Copy File  ${SUPPORT_FILES}/sophos_certs/rootca384.crt  ${SOPHOS_INSTALL}/base/update/rootcerts/rootca384.crt.0
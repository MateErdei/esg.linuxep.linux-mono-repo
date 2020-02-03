*** Settings ***
Documentation     Test the SUL Downloader

Library           Process
Library           OperatingSystem
Library           Collections

Library           ${libs_directory}/SulDownloader.py
Library           ${libs_directory}/UpdateServer.py
Library           ${libs_directory}/WarehouseGenerator.py

Resource          ../installer/InstallerResources.robot
Resource          ../GeneralTeardownResource.robot


Suite Setup       Generate Update Certs
Suite Teardown    Terminate All Processes    kill=True

Test Setup        Setup Tmpdir
Test Teardown     Teardown Tmpdir

*** Keywords ***

Setup Tmpdir
    Set Test Variable    ${tmpdir}     ./tmp
    Remove Directory   ${tmpdir}    recursive=True
    Create Directory   ${tmpdir}
    Require Fresh Install
    Install Ostia Certificate


Teardown Tmpdir
    General Test Teardown
    Variable Should Exist    ${tmpdir}
    Remove Directory   ${tmpdir}    recursive=True
    Remove Ostia Certificate

Install Ostia Certificate
    Copy File          SupportFiles/sophos_certs/OstiaCA.crt  /usr/local/share/ca-certificates
    File Should Exist  /usr/local/share/ca-certificates/OstiaCA.crt
    ${result} =        Run Process  update-ca-certificates
    Log    "stderr = ${result.stderr}"
    Should Be Equal As Integers   ${result.rc}   0


Remove Ostia Certificate
    Remove File     /usr/local/share/ca-certificates/OstiaCA.crt
    File Should Not Exist  /usr/local/share/ca-certificates/OstiaCA.crt
    ${result} =   Run Process  update-ca-certificates
    Log    "stderr = ${result.stderr}"
    Should Be Equal As Integers   ${result.rc}   0



*** Test Cases ***

# This is an old test to connect to ostia and download the SVE product from the warehouse
# It will only run on ubuntu as the other systems use a different method for ca-certificates
Can Connect to Ostia with System Certificate
    [Documentation]  Demonstrate that SulDownloader connects to Ostia using the :system: certificate and pre-hashed key and default certificate path
    [Tags]  MANUAL  SULDOWNLOADER
    ${config} =    Create JSON Config For Ostia Vshield
    
    Create File    ${tmpdir}/update_config.json    content=${config}

    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/output.json  stderr=${tmpdir}/download.log  stdout=${tmpdir}/download.log

    Log File  ${tmpdir}/download.log
    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    ${output} =    Get File    ${tmpdir}/output.json
    Should Contain    ${output}   PACKAGESOURCEMISSING


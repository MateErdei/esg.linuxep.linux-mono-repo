*** Settings ***
Library    Process
Library    OperatingSystem
Library    ${LIBS_DIRECTORY}/SulDownloader.py
Library    ${LIBS_DIRECTORY}/UpdateServer.py

Resource  ../watchdog/LogControlResources.robot

*** Keywords ***
Dump Suldownloader Log
    Should Exist   ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    ${contents} =   Get File  ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    Log   "suldownloader.log = ${contents}"
    Return from Keyword  ${contents}

Check Suldownloader Is Not Running
    ${result} =    Run Process  pgrep SulDownloader  shell=true
    Should Be Equal As Integers    ${result.rc}    1       msg="stdout:${result.stdout}\nerr: ${result.stderr}"

Setup Dev Certs for sdds3
    Copy File  ${SUPPORT_FILES}/sophos_certs/rootca384-local-only.crt  ${SOPHOS_INSTALL}/base/update/rootcerts/devrootca384.crt
    Copy File  ${SUPPORT_FILES}/sophos_certs/prod_certs/rootca384.crt  ${SOPHOS_INSTALL}/base/update/rootcerts/prodrootca384.crt

Setup Install SDDS3 Base
    Require Fresh Install
    Remove File    ${SOPHOS_INSTALL}/base/VERSION.ini.0
    Copy File  ${SYSTEMPRODUCT_TEST_INPUT}/sspl-base/VERSION.ini  ${SOPHOS_INSTALL}/base/VERSION.ini.0
    Setup Dev Certs for sdds3

Wait For Suldownloader To Finish
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check Suldownloader Is Not Running

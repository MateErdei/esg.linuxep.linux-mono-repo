*** Settings ***
Documentation    Product tests of SafeStore
Force Tags       INTEGRATION  SAFESTORE

Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVAndBaseResources.robot
Resource    ../shared/AVResources.robot
Resource    ../shared/ErrorMarkers.robot

Library         ../Libs/CoreDumps.py
Library         ../Libs/OnFail.py
Library         ../Libs/ProcessUtils.py

Test Setup      SafeStore Test Setup
Test Teardown   SafeStore Test TearDown

*** Variables ***
${CLEAN_STRING}     not an eicar
${NORMAL_DIRECTORY}     /home/vagrant/this/is/a/directory/for/scanning
${CUSTOMERID_FILE}  ${COMPONENT_ROOT_PATH}/chroot/${COMPONENT_ROOT_PATH}/var/customer_id.txt
${MACHINEID_CHROOT_FILE}  ${COMPONENT_ROOT_PATH}/chroot${SOPHOS_INSTALL}/base/etc/machine_id.txt
${MACHINEID_FILE}   ${SOPHOS_INSTALL}/base/etc/machine_id.txt


*** Test Cases ***

SafeStore Logs When It Recieves A File To Quarantine
    register cleanup    Exclude Watchdog Log Unable To Open File Error

    Mark AV Log
    Send Flags Policy To Base  flags_policy/flags_enabled.json
    Wait Until AV Plugin Log Contains With Offset
    ...   Safestore flag set. Setting Safestore to enabled.
    ...   timeout=60

    Check avscanner can detect eicar

    Wait Until Safestore Log Contains  Received Threat:


*** Keywords ***
SafeStore Test Setup
    Require Plugin Installed and Running  DEBUG
    Run Keyword and Ignore Error   Run Shell Process   ${SOPHOS_INSTALL}/bin/wdctl stop mcsrouter  OnError=Failed to stop mcsrouter
    Start SafeStore

    Mark AV Log
    Mark Sophos Threat Detector Log

    register on fail  dump log  ${THREAT_DETECTOR_LOG_PATH}
    register on fail  dump log  ${WATCHDOG_LOG}
    register on fail  dump log  ${SAFESTORE_LOG_PATH}
    register on fail  dump log  ${SOPHOS_INSTALL}/logs/base/wdctl.log
    register on fail  dump log  ${SOPHOS_INSTALL}/plugins/av/log/av.log
    register on fail  dump log   ${SUSI_DEBUG_LOG_PATH}
    register on fail  dump threads  ${SOPHOS_THREAT_DETECTOR_BINARY}
    register on fail  dump threads  ${PLUGIN_BINARY}

    Register Cleanup      Check All Product Logs Do Not Contain Error
    Register Cleanup      Exclude MCS Router is dead
    Register Cleanup      Exclude CustomerID Failed To Read Error
    Register Cleanup      Require No Unhandled Exception
    Register Cleanup      Check For Coredumps  ${TEST NAME}
    Register Cleanup      Check Dmesg For Segfaults

SafeStore Test TearDown
    run cleanup functions
    run failure functions if failed

    #restore machineID file
    Create File  ${MACHINEID_FILE}  3ccfaf097584e65c6c725c6827e186bb
    Remove File  ${CUSTOMERID_FILE}

    Stop SafeStore
    run keyword if test failed  Restart AV Plugin And Clear The Logs For Integration Tests
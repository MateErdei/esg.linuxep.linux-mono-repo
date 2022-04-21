*** Settings ***
Library         Process
Library         OperatingSystem
Library         String
Library         ../Libs/BaseUtils.py
Library         ../Libs/OnFail.py

Resource    GlobalSetup.robot

*** Keywords ***

Component Test Setup
    Run Keyword And Ignore Error   Empty Directory   ${SOPHOS_INSTALL}/tmp
    Start Fake Management If Required


Component Test TearDown
    # Stop Fake Management
    Terminate All Processes  kill=True
    Run Keyword If Test Failed  Run Keyword And Ignore Error  Log File   ${COMPONENT_ROOT_PATH}/log/${COMPONENT_NAME}.log  encoding_errors=replace
    Run Keyword If Test Failed  Run Keyword And Ignore Error  Log File   ${FAKEMANAGEMENT_AGENT_LOG_PATH}  encoding_errors=replace
    Run Keyword If Test Failed  Run Keyword And Ignore Error  Log File   ${THREAT_DETECTOR_LOG_PATH}  encoding_errors=replace


Setup Base And Component
    Mock Base Installation
    Setup Component For Testing


Mock Base Installation
    Run Keyword And Ignore Error  BaseUtils.uninstall_sspl_if_installed
    Create Directory   ${SOPHOS_INSTALL}
    Create Directory   ${SOPHOS_INSTALL}/base/lib64
    Create Directory   ${SOPHOS_INSTALL}/base/mcs/action
    Create Directory   ${SOPHOS_INSTALL}/base/mcs/policy
    Create Directory   ${SOPHOS_INSTALL}/base/telemetry/cache
    Copy File  ${BASE_SDDS}/files/base/lib64/libcrypto.so.1.1  ${SOPHOS_INSTALL}/base/lib64/
    Create Directory   ${SOPHOS_INSTALL}/tmp
    Create Directory   ${SOPHOS_INSTALL}/var/ipc
    Create Directory   ${SOPHOS_INSTALL}/var/ipc/plugins
    Set Base Log Level  DEBUG
    Run Process   groupadd  sophos-spl-group
    Run Process   useradd  sophos-spl-av    --no-create-home  --no-user-group  --gid  sophos-spl-group  --groups  sophos-spl-ipc  --system
    Run Process   useradd  sophos-spl-threat-detector  --no-create-home  --no-user-group  --gid  sophos-spl-group  --system
    Run Process   useradd  sophos-spl-user  --no-create-home  --no-user-group  --gid  sophos-spl-group  --system

Set Base Log Level
    [Arguments]  ${logLevel}
    Create File  ${SOPHOS_INSTALL}/base/etc/logger.conf  [global]\nVERBOSITY=${logLevel}

Set Log Level
    [Arguments]  ${logLevel}
    Create File  ${SOPHOS_INSTALL}/base/etc/logger.conf.local  [global]\nVERBOSITY=${logLevel}

Bootstrap SUSI From Update Source
    ${SUSI_UPDATE_SRC} =  Set Variable   ${COMPONENT_ROOT_PATH}/chroot/susi/update_source/
    ${SUSI_DIST_VERS} =   Set Variable   ${COMPONENT_ROOT_PATH}/chroot/susi/distribution_version/
    ${SUSI_V1DIR} =       Set Variable   ${SUSI_DIST_VERS}/version1/

    Copy Files  ${SUSI_UPDATE_SRC}/libsusi.so*                  ${SUSI_DIST_VERS}
    ${nonsupp_manifest} =  Get File   ${SUSI_UPDATE_SRC}/nonsupplement_manifest.txt
    Create File     ${SUSI_V1DIR}/package_manifest.txt   version1\n
    Append To File  ${SUSI_V1DIR}/package_manifest.txt   ${nonsupp_manifest}
    Append To File  ${SUSI_V1DIR}/package_manifest.txt   vdl 1\n
    Append To File  ${SUSI_V1DIR}/package_manifest.txt   model 1\n
    Append To File  ${SUSI_V1DIR}/package_manifest.txt   reputation 1\n

    Copy File   ${SUSI_V1DIR}/package_manifest.txt  ${COMPONENT_ROOT_PATH}/chroot/susi/update_source/package_manifest.txt

    Copy Files  ${SUSI_UPDATE_SRC}/version_manifest.txt*  ${SUSI_DIST_VERS}

    Copy Files  ${SUSI_UPDATE_SRC}/libglobalrep/*   ${SUSI_V1DIR}
    Copy Files  ${SUSI_UPDATE_SRC}/libsavi/*        ${SUSI_V1DIR}
    Copy Files  ${SUSI_UPDATE_SRC}/libsophtainer/*  ${SUSI_V1DIR}
    Copy Files  ${SUSI_UPDATE_SRC}/libupdater/*     ${SUSI_V1DIR}
    Copy Files  ${SUSI_UPDATE_SRC}/lrlib/*          ${SUSI_V1DIR}
    Copy Files  ${SUSI_UPDATE_SRC}/mllib/*          ${SUSI_V1DIR}
    Copy Files  ${SUSI_UPDATE_SRC}/susicore/*       ${SUSI_V1DIR}

    Copy Files  ${SUSI_UPDATE_SRC}/reputation/*     ${SUSI_V1DIR}/lrdata
    Copy Files  ${SUSI_UPDATE_SRC}/model/*          ${SUSI_V1DIR}/mlmodel
    Copy Files  ${SUSI_UPDATE_SRC}/vdl/*            ${SUSI_V1DIR}/vdb
    Copy Files  ${SUSI_UPDATE_SRC}/rules/*          ${SUSI_V1DIR}/rules

    Copy Files  ${BASE_SDDS}/files/base/lib64/libz.so.1.*   ${SUSI_V1DIR}
    Create File  ${SOPHOS_INSTALL}/base/etc/machine_id.txt  ab7b6758a3ab11ba8a51d25aa06d1cf4

    Run Process   ldconfig   -lN   *.so.*   cwd=${SUSI_DIST_VERS}   shell=True
    Run Process   ldconfig   -lN   *.so.*   cwd=${SUSI_V1DIR}   shell=True
    Run Process   ln  -s  libsophtainer.so.1  libsophtainer.so   cwd=${SUSI_V1DIR}   shell=True
    Run Process   ln  -s  libglobalrep.so.1  libglobalrep.so   cwd=${SUSI_V1DIR}   shell=True
    ${result} =  Run Process  ls  -l  ${TEST_INPUT_PATH}/vdl/
    Log  /opt/test/inputs/vdl: ${result.stdout}
    ${result} =  Run Process  find  ${SUSI_UPDATE_SRC}
    Log  Update Source: ${result.stdout}
    ${result} =  Run Process  find  ${SUSI_V1DIR}
    Log  Distribution Version: ${result.stdout}
    ${result} =  Run Process  ls  -l  ${SUSI_V1DIR}/vdb/
    Log  Virus Data: ${result.stdout}


Setup Component For Testing
    Run  pgrep -f sophos-spl | xargs kill -9
    Copy Directory   ${COMPONENT_SDDS}/files/plugins   ${SOPHOS_INSTALL}
    ## Change permissions for all executables
    Run Process  chmod  -R  +x  ${COMPONENT_ROOT_PATH}/sbin  ${COMPONENT_ROOT_PATH}/bin  ${BASH_SCRIPTS_PATH}  shell=True
    Run Process  chmod  +x  ${COMPONENT_ROOT_PATH}/sophos_certs/InstallCertificateToSystem.sh
    Create Directory  ${COMPONENT_ROOT_PATH}/chroot/opt/sophos-spl/base/etc
    Create Directory  ${COMPONENT_ROOT_PATH}/chroot/opt/sophos-spl/base/update/var
    Create Directory  ${COMPONENT_ROOT_PATH}/chroot/etc
    Create Directory  ${COMPONENT_ROOT_PATH}/chroot/log
    Create Directory  ${COMPONENT_ROOT_PATH}/chroot/tmp
    Create Directory  ${COMPONENT_ROOT_PATH}/chroot/var
    Create Directory  ${COMPONENT_ROOT_PATH}/chroot/${COMPONENT_ROOT_PATH}
    Create Directory  ${COMPONENT_ROOT_PATH}/chroot/${COMPONENT_ROOT_PATH}/log
    Create Directory  ${COMPONENT_ROOT_PATH}/chroot/${COMPONENT_ROOT_PATH}/var
    Create Directory  ${COMPONENT_ROOT_PATH}/var
    Create Directory  ${COMPONENT_ROOT_PATH}/log
    Run Process   ln  -s  /  ${COMPONENT_ROOT_PATH}/chroot${COMPONENT_ROOT_PATH}/chroot
    Run Process   ln  -snf  ${COMPONENT_ROOT_PATH}/chroot/log  ${COMPONENT_ROOT_PATH}/log/sophos_threat_detector
    ${RELATIVE_PATH} =   Replace String Using Regexp   ${COMPONENT_ROOT_PATH}/log   [^/]+   ..
    ${RELATIVE_PATH} =   Strip String   ${RELATIVE_PATH}   characters=/
    Run Process   ln  -snf  ${RELATIVE_PATH}/log  ${COMPONENT_ROOT_PATH}/chroot/${COMPONENT_ROOT_PATH}/log/sophos_threat_detector
    Run Process   ldconfig   -lN   *.so.*   cwd=${COMPONENT_LIB64_DIR}   shell=True
    Bootstrap SUSI From Update Source

Use Fake AVScanner
    Set Environment Variable  ${USING_FAKE_AV_SCANNER_FLAG}  true
    Move File  ${COMPONENT_ROOT_PATH}/sbin/scheduled_file_walker_launcher  ${COMPONENT_ROOT_PATH}/sbin/scheduled_file_walker_launcher_bkp
    Copy File  ${RESOURCES_PATH}/copyScanConfigFilesToTmp.sh   ${COMPONENT_ROOT_PATH}/sbin/scheduled_file_walker_launcher
    Run  chmod 755 ${COMPONENT_ROOT_PATH}/sbin/scheduled_file_walker_launcher
    register cleanup  Undo Use Fake AVScanner

Undo Use Fake AVScanner
    ${usingFakeAVScanner} =  Get Environment Variable  ${USING_FAKE_AV_SCANNER_FLAG}
    Return From Keyword If  '${usingFakeAVScanner}'!='true'

    Set Environment Variable  ${USING_FAKE_AV_SCANNER_FLAG}  false
    Remove File  ${COMPONENT_ROOT_PATH}/sbin/scheduled_file_walker_launcher
    Move File  ${COMPONENT_ROOT_PATH}/sbin/scheduled_file_walker_launcher_bkp  ${COMPONENT_ROOT_PATH}/sbin/scheduled_file_walker_launcher
    Remove Directory  /tmp/config-files-test/  recursive=true

Delete Eicars From Tmp
    Remove File  /tmp_test/*eicar*

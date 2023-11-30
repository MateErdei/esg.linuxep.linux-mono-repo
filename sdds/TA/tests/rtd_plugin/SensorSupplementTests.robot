*** Settings ***
Resource    ${COMMON_TEST_ROBOT}/UpgradeResources.robot
Resource    ${COMMON_TEST_ROBOT}/GeneralUtilsResources.robot

Library         DateTime
Library         OperatingSystem
Library         Process

Library    ${COMMON_TEST_LIBS}/LogUtils.py
Library    ${COMMON_TEST_LIBS}/OnFail.py
Library    ${COMMON_TEST_LIBS}/MCSRouter.py

Force Tags    TAP_PARALLEL2

Suite Setup     Sdds3 Suite Setup
Suite Teardown  Sdds3 Suite Teardown

Test Setup      Sdds3 Test Setup
Test Teardown   Sdds3 Test Teardown


*** Variables ***
${BASE_LOGS}                    ${SOPHOS_INSTALL}/logs/base
${SUL_DOWNLOADER_LOG}           ${BASE_LOGS}/suldownloader.log
${UPDATE_SCHED_LOG}             ${BASE_LOGS}/updatescheduler.log
${RUNTIME_DETECTIONS_CONTENT}   ${RTD_DIR}/etc/content_rules/rules.yaml
${RDRULES_SCIT}                 ${INPUT_DIRECTORY}/sspl_rdrules_scit/supplement/rd_rules
${CONTENT_TAG}                  2023-3
${SUPPLEMENT_SRC}               ${RDRULES_SCIT}/content/${CONTENT_TAG}
${SENSOR_ETC_FOLDER}            ${RTD_DIR}/etc
${CONTENT_BPF_DIR}              ${SENSOR_ETC_FOLDER}/content_rules/bpf
${SUPPLEMENT_DIR}               /tmp/rdrules
${RUNTIME_DETECTIONS_LOG_PATH}  ${RTD_DIR}/log/runtimedetections.log
${RUNTIME_DETECTIONS_BIN_PATH}  ${RTD_DIR}/bin/runtimedetections
${SDDS3_REPO}                   ${INPUT_DIRECTORY}/repo

*** Test Cases ***
Sdds3 Supplement Update Changes Content
    # TODO once 2023.43/2023.4 is in dogfood: remove ARM64 exclusion
    [Tags]    EXCLUDE_ARM64
    [Documentation]
    ...    Updates from a modified content supplement. Verifies that changes to
    ...    the yaml are applied and that BPF programs can be added, modified & removed.

    Mark Updater Log

    Start Local SDDS3 Server
    configure_and_run_SDDS3_thininstaller    ${0}    https://localhost:8080    https://localhost:8080    thininstaller_source=${THIN_INSTALLER_DIRECTORY}

    Check Installed and Running

    File Should Not Exist    ${CONTENT_BPF_DIR}/test-a.bpf.o
    File Should Not Exist    ${CONTENT_BPF_DIR}/test-b.bpf.o
    File Should Not Exist    ${CONTENT_BPF_DIR}/test-c.bpf.o
    File Should Not Exist    ${CONTENT_BPF_DIR}/test-d.bpf.o

    # first update

    Copy Supplement
    File Should Exist    ${SUPPLEMENT_DIR}/runtimedetections-content.yaml
    Append To File    ${SUPPLEMENT_DIR}/runtimedetections-content.yaml    \n#first update\n
    Create File   ${SUPPLEMENT_DIR}/bpf/test-a.bpf.o   test-a
    Create File   ${SUPPLEMENT_DIR}/bpf/test-b.bpf.o   test-b
    Create File   ${SUPPLEMENT_DIR}/bpf/test-c.bpf.o   test-c
    Rebuild Supplement

#    Add dev updating certs to installation
    Run Update Now

    Check Installed and Running

    Grep File    ${RUNTIME_DETECTIONS_CONTENT}    \#first update
    Files Should Be Identical
    ...    ${SUPPLEMENT_DIR}/runtimedetections-content.yaml
    ...    ${RUNTIME_DETECTIONS_CONTENT}

    List Directory   ${CONTENT_BPF_DIR}
    File Exists And Contains   ${CONTENT_BPF_DIR}/test-a.bpf.o   test-a
    File Exists And Contains   ${CONTENT_BPF_DIR}/test-b.bpf.o   test-b
    File Exists And Contains   ${CONTENT_BPF_DIR}/test-c.bpf.o   test-c
    File Should Not Exist    ${CONTENT_BPF_DIR}/test-d.bpf.o

    # second update

    Append To File    ${SUPPLEMENT_DIR}/runtimedetections-content.yaml    \n#second update\n
    Remove File   ${SUPPLEMENT_DIR}/bpf/test-a.bpf.o
    Create File   ${SUPPLEMENT_DIR}/bpf/test-c.bpf.o   test-cc
    Create File   ${SUPPLEMENT_DIR}/bpf/test-d.bpf.o   test-d
    Rebuild Supplement

    Run Update Now

    Check Installed and Running

    Grep File    ${RUNTIME_DETECTIONS_CONTENT}    \#second update
    Files Should Be Identical
    ...    ${SUPPLEMENT_DIR}/runtimedetections-content.yaml
    ...    ${RUNTIME_DETECTIONS_CONTENT}

    List Directory   ${CONTENT_BPF_DIR}
    File Should Not Exist    ${CONTENT_BPF_DIR}/test-a.bpf..o
    File Exists And Contains   ${CONTENT_BPF_DIR}/test-b.bpf.o   test-b
    File Exists And Contains   ${CONTENT_BPF_DIR}/test-c.bpf.o   test-cc
    File Exists And Contains   ${CONTENT_BPF_DIR}/test-d.bpf.o   test-d


*** Keywords ***
Sdds3 Suite Setup
    Setup_MCS_Cert_Override
    Run Keyword And Ignore Error    Uninstall Base
    Remove Directory   ${SENSOR_ETC_FOLDER}   recursive=True
    Remove Directory  /var/lib/sophos  recursive=True
    Remove Directory  /var/run/sophos  recursive=True

    start local cloud server

Sdds3 Suite Teardown
    [Timeout]   1 min
    stop local cloud server
    Stop Local SDDS3 Server

Sdds3 Test Setup
    Directory Should Not Exist    ${SOPHOS_INSTALL}

Sdds3 Test Teardown
    [Timeout]   2 mins
    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain String    FATAL

    Run Keyword If Test Failed  dump all logs

    # Run "teardown functions" which might include a restart after dumping all the logs
    Run Teardown Functions

    Run Keyword And Ignore Error   Uninstall SPL

Run Update Now
    [Documentation]
    ...   Trigger an update via Fake Cloud, watch the updater log for
    ...   the update to complete, then check for success.
    Mark Updater Log
    Trigger Update Now
    Wait for Update Success
    Dump Marked Log    ${SUL_DOWNLOADER_LOG}

Uninstall Base
    ${result} =   Run Process  bash ${SOPHOS_INSTALL}/bin/uninstall.sh --force   shell=True   timeout=30s
    Should Be Equal As Integers  ${result.rc}  0   "Failed to uninstall base.\nstdout: \n${result.stdout}\n. stderr: \n${result.stderr}"

Uninstall SPL
    Directory Should Exist    ${SOPHOS_INSTALL}
    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain String    FATAL
    Uninstall Base
    Directory Should Not Exist    ${SOPHOS_INSTALL}

Configure Updating
    [Documentation]
    ...   Apply config copied from the "Sdds3 Install" test
    Copy File   ${RESOURCES_PATH}/update_config/mcs.config  ${SOPHOS_INSTALL}/base/etc/
    Copy File   ${RESOURCES_PATH}/update_config/sophosspl_mcs.config   ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config
    Copy File   ${RESOURCES_PATH}/update_config/sdds3_override_settings.ini   ${SOPHOS_INSTALL}/base/update/var
    Copy File   ${RESOURCES_PATH}/update_config/ALC-1_policy.xml   ${SOPHOS_INSTALL}/base/mcs/policy

Mark Updater Log
    ${mark}=   Mark Log Size    logpath=${SUL_DOWNLOADER_LOG}
    Set Test Variable    $SULDOWNLOADER_LOG_MARK   ${mark}

Wait for Update Success
    [Documentation]
    ...    Watches the updater log. Wait for update to complete,
    ...    then check for success.
    Wait For Log Contains After Mark
    ...    logpath=${SUL_DOWNLOADER_LOG}
    ...    expected=Running SDDS3 update
    ...    mark=${SULDOWNLOADER_LOG_MARK}
    ...    timeout=10

    Wait For Log Contains After Mark
    ...    logpath=${SUL_DOWNLOADER_LOG}
    ...    expected=SUS Request was successful
    ...    mark=${SULDOWNLOADER_LOG_MARK}
    ...    timeout=10

    Wait For Log Contains After Mark
    ...    logpath=${SUL_DOWNLOADER_LOG}
    ...    expected=Performing Sync using https://localhost:8080
    ...    mark=${SULDOWNLOADER_LOG_MARK}
    ...    timeout=10

    Wait For Log Contains After Mark
    ...    logpath=${SUL_DOWNLOADER_LOG}
    ...    expected=Generating the report file
    ...    mark=${SULDOWNLOADER_LOG_MARK}
    ...    timeout=120

    Check Log Contains After Mark
    ...    log_path=${SUL_DOWNLOADER_LOG}
    ...    expected=Update success
    ...    mark=${SULDOWNLOADER_LOG_MARK}

Check Installed and Running
    Directory Should Exist    ${SOPHOS_INSTALL}
    Directory Should Exist    ${RTD_DIR}
    File Should Exist    ${RUNTIME_DETECTIONS_BIN_PATH}
    File Should Exist    ${RUNTIME_DETECTIONS_CONTENT}

    Wait For Rtd Log Contains After Last Restart    ${RUNTIME_DETECTIONS_LOG_PATH}    Analytics started processing telemetry   timeout=${30}
    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain String    FATAL

Copy Supplement
    [Arguments]
    ...    ${src}=${SUPPLEMENT_SRC}
    ...    ${dst}=/tmp/rdrules
    Remove Directory   ${dst}   recursive=True
    Register Cleanup If Unique   Remove Directory   ${dst}   recursive=True
    Copy Directory   source=${src}   destination=${dst}
    Set Test Variable   $SUPPLEMENT_DIR   ${dst}

Rebuild Supplement
    [Arguments]   ${src}=/tmp/rdrules   ${tag}=${CONTENT_TAG}
    ${SUPPLEMENT_FILE}=   Set Variable   ${SDDS3_REPO}/supplement/sdds3.RuntimeDetectionRules.dat
    File Should Exist   ${SUPPLEMENT_FILE}

    IF   ${{os.path.isfile("${SUPPLEMENT_FILE}.bak")}}
        Remove File   ${SUPPLEMENT_FILE}
    ELSE
        Move File   ${SDDS3_REPO}/supplement/sdds3.RuntimeDetectionRules.dat   ${SDDS3_REPO}/supplement/sdds3.RuntimeDetectionRules.dat.bak
        Register Cleanup   Move File   ${SDDS3_REPO}/supplement/sdds3.RuntimeDetectionRules.dat.bak   ${SDDS3_REPO}/supplement/sdds3.RuntimeDetectionRules.dat
    END

    Make Sdds3 Supplement   src=${src}   dst=${SDDS3_REPO}   tag=${tag}

    File Should Exist   ${SDDS3_REPO}/supplement/sdds3.RuntimeDetectionRules.dat

Make Sdds3 Supplement
    [Arguments]
    ...    ${tag}
    ...    ${src}
    ...    ${dst}=/tmp
    ...    ${timestamp}=${None}
    ...    ${version}=9.9.9.9
    ...    ${nonce}=ffffffffff

    IF   ${timestamp} is ${None}
        #${timestamp}=   Set Variable   2099-12-31T23:59:59Z
        ${timestamp}=   Get Current Date   result_format=%Y-%m-%dT%H:%M:%SZ
    END

    Directory Should Exist    ${src}

    ## create package/LocalRepData_2099.12.31.9.0.0.1.ffffffffff.zip
    # /opt/SDDS3/sdds3-builder --build-stub-package --package-dir /opt/SDDS3/repo/package --stub-dir /tmp/LocalRepData --name LocalRepData -version 2099.12.31.9.0.0.1 --nonce ffffffffff

    ${result}=   Run Process
    ...    ${SDDS3_Builder}
    ...    --build-stub-package
    ...    --package-dir   ${dst}/package
    ...    --stub-dir   ${src}
    ...    --name   RuntimeDetectionRules
    ...    --version   ${version}
    ...    --nonce   ${nonce}
    ...    stderr=STDOUT
    Log   ${result.stdout}

    ${package_name}=   Set Variable   RuntimeDetectionRules_${version}.${nonce}.zip
    ${package_path}=   Set Variable   ${dst}/package/${package_name}
    File Should Exist    ${package_path}

    ## create supplement/sdds3.LocalRepData.dat
    # /opt/SDDS3/sdds3-builder --build-supplement --dir /opt/SDDS3/repo/supplement --xml /tmp/LocalRep.xml --name LocalRepData --version 2099.12.31.9.0.0.1 --nonce ffffffffff

    ${sha256}=   Get File Sha256   ${package_path}
    ${size}=   Get File Size   ${package_path}
    ${xml}=   Catenate   SEPARATOR=${\n}
    ...   <?xml version="1.0" encoding="UTF-8" standalone="yes"?>
    ...   <supplement name="RuntimeDetectionRules" timestamp="${timestamp}">
    ...     <package-ref src="${package_name}" size="${size}" sha256="${sha256}">
    ...       <name>RuntimeDetectionRules</name>
    ...       <version>${version}</version>
    ...       <line-id>RuntimeDetectionRules</line-id>
    ...       <nonce>${nonce}</nonce>
    ...       <description>RuntimeDetectionRules Supplement</description>
    ...       <tags>
    ...         <tag name="${tag}">
    ...           <release-group name="0" />
    ...           <release-group name="1" />
    ...           <release-group name="A" />
    ...           <release-group name="B" />
    ...           <release-group name="C" />
    ...         </tag>
    ...       </tags>
    ...     </package-ref>
    ...   </supplement>
    Create File   /tmp/sdds3.RuntimeDetectionRules.xml   ${xml}

    ${result}=   Run Process
    ...    ${SDDS3_Builder}
    ...    --build-supplement
    ...    --dir   ${dst}/supplement
    ...    --xml   /tmp/sdds3.RuntimeDetectionRules.xml
    ...    --name   RuntimeDetectionRules
    ...    --version   ${version}
    ...    --nonce   ${nonce}
    ...    --verbose
    ...    stderr=STDOUT
    Log   ${result.stdout}

    File Should Exist   ${dst}/supplement/sdds3.RuntimeDetectionRules.dat


Files Should Be Identical
    [Arguments]   ${file1}   ${file2}
    ${result}=   Run Process
    ...    diff   ${file1}   ${file2}
    ...    stderr=STDOUT
    ...    timeout=30
    IF   ${result.rc} != ${0}
        Log   ${result.stdout}
        Fail   Files ${file1} and ${file2} are not identical
    END

Directories Should Be Identical
    [Arguments]   ${dir1}   ${dir2}
    ${result}=   Run Process
    ...    diff   --recursive   ${dir1}   ${dir2}
    ...    stderr=STDOUT
    ...    timeout=30
    IF   ${result.rc} != ${0}
        Log   ${result.stdout}
        Fail   Directories ${dir1} and ${dir2} are not identical
    END

Get File Sha256
    [Arguments]  ${path}
    ${result} =  Run Process  sha256sum  -b  ${path}
    Log  ${result.stdout}
    Log  ${result.stderr}
    @{parts} =  Split String  ${result.stdout}
    [Return]  ${parts}[0]

File Exists And Contains
    [Arguments]   ${file}   ${expected}
    File Should Exist    ${file}
    ${contents}=   Get File   ${file}
    Should Be Equal As Strings    ${contents}    ${expected}
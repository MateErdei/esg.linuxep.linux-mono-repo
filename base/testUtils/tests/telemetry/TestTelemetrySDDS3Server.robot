
*** Settings ***
Suite Setup      Upgrade Resources Suite Setup
Suite Teardown   Upgrade Resources Suite Teardown

Test Setup       Require Uninstalled
Test Teardown    Run Keywords
...                Remove Environment Variable  http_proxy    AND
...                Remove Environment Variable  https_proxy  AND
...                Stop Proxy If Running    AND
...                Stop Proxy Servers   AND
...                Clean up fake warehouse  AND
...                Upgrade Resources SDDS3 Test Teardown

Resource    TelemetryResources.robot
Resource    ../update/SDDS3Resources.robot
Resource    ../upgrade_product/UpgradeResources.robot

*** Variables ***


*** Test Cases ***
Telemetry Executable Moves All Top Level Telemetry Items ESM Enabled
    Cleanup Telemetry Server

    Setup SUS static
    ${fixed_version_token} =    read_token_from_warehouse_linuxep_json    ${tmpLaunchDarkly}/${staticflagfile}
    ${fixed_version_name} =    read_name_from_warehouse_linuxep_json    ${tmpLaunchDarkly}/${staticflagfile}
    ${esm_enabled_alc_policy} =    populate_fixed_version_with_normal_cloud_sub    ${fixed_version_name}    ${fixed_version_token}

    Create File  ${tmpPolicy}   ${esm_enabled_alc_policy}
    Register Cleanup  Remove File  ${tmpPolicy}

    Start Local Cloud Server  --initial-alc-policy  ${tmpPolicy}

    # LINUXDAR-7059: On SUSE the thin installer fails to connect to the first SDDS3 server so workaround for now by running twice
    ${result} =  Run Process    bash -x ${SUPPORT_FILES}/jenkins/runCommandFromPythonVenvIfSet.sh python3 ${LIBS_DIRECTORY}/SDDS3server.py --launchdarkly ${tmpLaunchDarkly} --sdds3 ${VUT_WAREHOUSE_ROOT}/repo  shell=true    timeout=10s
    Log  ${result.stdout}
    Log  ${result.stderr}
    ${handle}=  Start Process  bash -x ${SUPPORT_FILES}/jenkins/runCommandFromPythonVenvIfSet.sh python3 ${LIBS_DIRECTORY}/SDDS3server.py --launchdarkly ${tmpLaunchDarkly} --sdds3 ${VUT_WAREHOUSE_ROOT}/repo  shell=true

    Set Suite Variable    ${GL_handle}    ${handle}

    configure_and_run_SDDS3_thininstaller  0  https://localhost:8080   https://localhost:8080
    Wait Until Keyword Succeeds
    ...   80 secs
    ...   10 secs
    ...   Check Log Contains String At Least N times    ${SULDownloaderLog}    suldownloader_log   Update success  2
    Check Current Release With AV Installed Correctly

    File Should Exist     ${SOPHOS_INSTALL}/base/update/var/package_config.xml
    Prepare To Run Telemetry Executable

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    LOG    ${telemetryFileContents}

    check_update_scheduler_telemetry_json_is_correct  ${telemetryFileContents}  0   True
        ...    install_state=0
        ...    download_state=0
        ...    base_tag=${fixed_version_name}
    check_base_telemetry_json_is_correct  ${telemetryFileContents}

    @{top_level_list} =  Create List   esmName    esmToken    suiteVersion    tenantId    deviceId
    check_top_level_telemetry_items    ${telemetryFileContents}   ${top_level_list}

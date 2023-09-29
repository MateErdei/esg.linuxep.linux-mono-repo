*** Settings ***
Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py
Resource  ../upgrade_product/UpgradeResources.robot


*** Variables ***
# Telemetry variables
${RTD_PLUGIN_PATH}     ${SOPHOS_INSTALL}/plugins/runtimedetections
${RTD_EXECUTABLE}      ${RTD_PLUGIN_PATH}/bin/runtimedetections

*** Keywords ***
Wait For RuntimeDetections to be Installed
    Wait Until Created   ${RTD_EXECUTABLE}   timeout=40s

    Wait Until Keyword Succeeds
    ...   40 secs
    ...   1 secs
    ...   RuntimeDetections Plugin Is Running

RuntimeDetections Plugin Is Running
    ${result} =    Run Process  pgrep  -f  ${RTD_EXECUTABLE}
    Should Be Equal As Integers    ${result.rc}    0   RuntimeDetections Plugin not running

RuntimeDetections Plugin Is Not Running
    ${result} =    Run Process  pgrep  -f  ${RTD_EXECUTABLE}
    Should Not Be Equal As Integers    ${result.rc}    0   RuntimeDetections Plugin still running

Install RuntimeDetections Directly
    ${RTD_SDDS_DIR} =   Get SSPL RuntimeDetections Plugin SDDS
    ${result} =   Run Process  bash -x ${RTD_SDDS_DIR}/install.sh 2> /tmp/rtd_install.log   shell=True
    ${stderr} =   Get File  /tmp/rtd_install.log
    Should Be Equal As Integers    ${result.rc}    0   "Installer failed: Reason ${result.stderr} ${stderr}"
    Log  ${result.stdout}
    Log  ${stderr}
    Log  ${result.stderr}
    Wait For RuntimeDetections to be Installed

Check RuntimeDetections Installed Correctly
    Verify Component Permissions
    Verify Running Component Permissions

Verify Permissions
    [Arguments]  ${path}  ${mode}   ${user}=${EMPTY}   ${group}=${EMPTY}
    ${stat_result} =    Evaluate   os.stat("${path}")

    ${actual_mode} =   Evaluate   oct(stat.S_IMODE(${stat_result.st_mode}))
    Should Be Equal    ${actual_mode}    ${mode}     msg="incorrect mode for ${path}"

    IF   '${user}' != '${EMPTY}'
        ${actual_user} =   Evaluate   pwd.getpwuid(${stat_result.st_uid}).pw_name
        Should Be Equal   ${actual_user}   ${user}     msg="incorrect user for ${path}"
    END

    IF   '${group}' != '${EMPTY}'
        ${actual_group} =   Evaluate   grp.getgrgid(${stat_result.st_gid}).gr_name
        Should Be Equal   ${actual_group}   ${group}     msg="incorrect group for ${path}"
    END

Verify Component Permissions
    ${result} =   Run Process
    ...    find ${RTD_PLUGIN_PATH} | xargs ls -ld
    ...    shell=True   stderr=STDOUT
    Log   ${result.stdout}

    Verify Permissions  ${RTD_PLUGIN_PATH}/  0o750    root   sophos-spl-group

    Verify Permissions  ${RTD_PLUGIN_PATH}/bin/  0o710    root   sophos-spl-group
    Verify Permissions  ${RTD_PLUGIN_PATH}/bin/runtimedetections  0o710    root   sophos-spl-group
    Verify Permissions  ${RTD_PLUGIN_PATH}/bin/uninstall.sh  0o710    root   sophos-spl-group
    
    Verify Permissions  ${RTD_PLUGIN_PATH}/etc  0o750    root   sophos-spl-group
    Verify Permissions  ${RTD_PLUGIN_PATH}/etc/sensor.yaml  0o640    root   sophos-spl-group
    Verify Permissions  ${RTD_PLUGIN_PATH}/etc/analytics.yaml  0o640    root   sophos-spl-group
    Verify Permissions  ${RTD_PLUGIN_PATH}/etc/content_rules/rules.yaml  0o640    root   sophos-spl-group

    Verify Permissions  ${RTD_PLUGIN_PATH}/log/  0o700    sophos-spl-user   sophos-spl-group
    Verify Permissions  ${RTD_PLUGIN_PATH}/log/runtimedetections.log  0o600    sophos-spl-user   sophos-spl-group

    Verify Permissions  ${RTD_PLUGIN_PATH}/var/  0o750    root   sophos-spl-group

    Verify Permissions  ${RTD_PLUGIN_PATH}/var/lib/  0o750    root   sophos-spl-group
    Verify Permissions  ${RTD_PLUGIN_PATH}/var/lib/struct_layouts.json  0o640    root   sophos-spl-group

    Verify Permissions  ${RTD_PLUGIN_PATH}/var/lib/bpf/  0o750    root   sophos-spl-group
    Verify Permissions  ${RTD_PLUGIN_PATH}/var/lib/bpf/dummy.bpf.o  0o640    root   sophos-spl-group
    Verify Permissions  ${RTD_PLUGIN_PATH}/var/lib/bpf/dummy.bpf.c  0o640    root   sophos-spl-group

    Verify Permissions  ${RTD_PLUGIN_PATH}/var/run/  0o750    sophos-spl-user   sophos-spl-group

    Verify Permissions  ${RTD_PLUGIN_PATH}/VERSION.ini  0o640    root   sophos-spl-group

    # if the content supplement includes BPF, verify that it was installed
    ${RTD_SDDS_DIR} =   Get SSPL RuntimeDetections Plugin SDDS
    IF   ${{os.path.isdir("${RTD_SDDS_DIR}/content_rules/bpf")}}
        Verify Permissions  ${RTD_PLUGIN_PATH}/etc/content_rules/bpf  0o750    root   sophos-spl-group

        @{BPF_OBJS}=   List Files In Directory   ${RTD_PLUGIN_PATH}/etc/content_rules/bpf   *.bpf.o
        IF   @{BPF_OBJS} != @{EMPTY}
            # Verify permissions of the first object we find
            ${BPF_OBJ}=   Get From List   ${BPF_OBJS}   0
            Verify Permissions  ${RTD_PLUGIN_PATH}/etc/content_rules/bpf/${BPF_OBJ}  0o640    root   sophos-spl-group
            ${BPF_SRC}=   Replace String Using Regexp    ${BPF_OBJ}    .o$    .c
            Verify Permissions  ${RTD_PLUGIN_PATH}/etc/content_rules/bpf/${BPF_SRC}  0o640    root   sophos-spl-group
        END
    END

Verify Running Component Permissions
    Verify Permissions   ${RTD_PLUGIN_PATH}/var/run/cache_analytics.yaml  0o640    sophos-spl-user   sophos-spl-group

    Verify Permissions   /var/run/sophos/   0o700   sophos-spl-user   sophos-spl-group
    Verify Permissions   /run/sophos/   0o700   sophos-spl-user   sophos-spl-group

    IF   ${{os.path.isdir("/sys/fs/cgroup/")}}
        IF   ${{os.path.isdir("/sys/fs/cgroup/systemd/runtimedetections/")}}
            Verify Permissions   /sys/fs/cgroup/systemd/runtimedetections/   0o755    sophos-spl-user   sophos-spl-group
        ELSE IF   ${{os.path.isdir("/sys/fs/cgroup/runtimedetections/")}}
            Verify Permissions   /sys/fs/cgroup/runtimedetections/   0o755    sophos-spl-user   sophos-spl-group
        ELSE
            Fail   Cannot find cgroup directory for runtimedetections
        END
    END

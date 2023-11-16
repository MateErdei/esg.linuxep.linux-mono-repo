# Copyright 2023 Sophos Limited. All rights reserved.

import json
from typing import Dict, Any, List

import tap.v1 as tap
from tap._backend import Input
from tap._pipeline.tasks import ArtisanInput
from dotdict import DotDict

from pipeline.common import (
    unified_artifact,
    get_test_machines,
    get_robot_args,
    TEST_TASK_TIMEOUT_MINUTES,
    get_test_builds,
    get_build_output_for_arch,
    stage_tap_machine,
    stage_task,
    run_pytest_tests,
    run_robot_tests,
)


def include_cifs_for_machine_name(template):
    no_samba = ("ubuntu2204",)
    distro = template.split("_")[0]
    return distro not in no_samba


def include_ntfs_for_machine_name(template):
    no_ntfs = ("amzlinux2023", "oracle79", "oracle87", "rhel79", "rhel87", "rhel91", "sles12", "sles15")
    distro = template.split("_")[0]
    if distro in no_ntfs:
        return False
    return True


def is_debian_based(template):
    distro = template.split("_")[0]
    return distro in (
        "debian10",
        "debian11",
        "ubuntu1804",
        "ubuntu2004",
        "ubuntu2204",
    )


def is_redhat_based(template):
    distro = template.split("_")[0]
    return distro in (
        "amzlinux2023",
        "amzlinux2",
        "centos7",
        "centos8stream",
        "centos9stream",
        "oracle79",
        "oracle87",
        "rhel79",
        "rhel87",
        "rhel91",
    )


def is_suse_based(template):
    distro = template.split("_")[0]
    return distro in (
        "sles12",
        "sles15",
    )


def get_inputs(context: tap.PipelineContext, build_output: ArtisanInput, build: str) -> Dict[str, Input]:
    supplement_branch = "released"

    test_inputs = dict(
        test_scripts=context.artifact.from_folder("./av/TA"),
        local_rep=context.artifact.from_component("ssplav-localrep", supplement_branch, None) / "reputation",
        dataseta=context.artifact.from_component("ssplav-dataseta", supplement_branch, None) / "dataseta",
        SupportFiles=context.artifact.from_folder("./base/testUtils/SupportFiles"),
    )

    test_inputs["tap_test_output"] = build_output / f"av/{build}/tap_test_output"
    test_inputs["av/SDDS-COMPONENT"] = build_output / f"av/{build}/installer"
    test_inputs["av/base-sdds"] = build_output / f"base/{build}/installer"

    arch = build.split("_")[1]
    if arch == "x64":
        test_inputs["ml_model"] = context.artifact.from_component("ssplav-mlmodel3-x86_64", "released") / "model"
    elif arch == "arm64":
        test_inputs["ml_model"] = context.artifact.from_component("ssplav-mlmodel3-arm64", "released") / "model"

    return test_inputs


@tap.timeout(task_timeout=TEST_TASK_TIMEOUT_MINUTES)
def run_av_component_tests(machine: tap.Machine):
    run_pytest_tests(
        machine, [""], scripts="test_scripts", extra_pytest_args_behind_paths=["--html=/opt/test/logs/log.html"]
    )


@tap.timeout(task_timeout=TEST_TASK_TIMEOUT_MINUTES)
def run_av_integration_tests(machine: tap.Machine, robot_args_json: str):
    robot_exclusion_tags = []
    platform = machine.template.split("_")[0]
    if platform in ("centos9stream", "ubuntu2204"):
        #  As of 2023-06-15 CentOS 9 Stream doesn't support NFSv2
        #  As of 2023-10-27 Ubuntu 22.04 doesn't support NFSv2
        robot_exclusion_tags.append("nfsv2")

    os_packages = []

    if include_cifs_for_machine_name(machine.template):
        print("CIFS enabled:", machine.template, id(machine))
        os_packages += ["samba", "cifs-utils"]
    else:
        print("CIFS disabled:", machine.template, id(machine))
        robot_exclusion_tags.append("cifs")

    if include_ntfs_for_machine_name(machine.template):
        print("NTFS enabled:", machine.template, id(machine))
        os_packages.append("ntfs-3g")
    else:
        print("NTFS disabled:", machine.template, id(machine))
        robot_exclusion_tags.append("ntfs")

    os_packages += ["zip", "gdb", "autofs", "util-linux"]
    if is_debian_based(machine.template):
        os_packages += ["nfs-kernel-server", "bfs", "libguestfs-reiserfs", "netcat"]
    elif is_redhat_based(machine.template):
        os_packages += ["nfs-utils", "nc", "bzip2"]
    elif is_suse_based(machine.template):
        os_packages += ["nfs-kernel-server", "libcap-progs", "netcat-openbsd"]
    else:
        raise Exception(f"{machine.template} has an unknown package manager")

    install_command = ["bash", machine.inputs.SupportFiles / "install_os_packages.sh"] + os_packages
    machine.run(*install_command, timeout=600)

    extra_robot_args = []
    if len(robot_exclusion_tags) > 0:
        extra_robot_args += ["--exclude"] + robot_exclusion_tags

    run_robot_tests(machine, robot_args=json.loads(robot_args_json) + extra_robot_args)


def stage_av_tests(
    stage: tap.Root,
    context: tap.PipelineContext,
    component: tap.Component,  # pylint: disable=unused-argument
    parameters: DotDict,
    outputs: Dict[str, Any],
    coverage_tasks: List[str],
):
    group_name = "av_tests"

    default_include_tags = "TAP_PARALLEL1,TAP_PARALLEL2,TAP_PARALLEL3,TAP_PARALLEL4,TAP_PARALLEL5,TAP_PARALLEL6"

    with stage.parallel(group_name):
        for build in get_test_builds():
            build_output = get_build_output_for_arch(outputs, build)
            if not build_output:
                continue
            inputs = get_inputs(context, build_output, build)
            test_machines = get_test_machines(build, parameters)

            robot_args = get_robot_args(parameters)
            includedtags = parameters.include_tags or default_include_tags

            for include in includedtags.split(","):
                for machine in test_machines:
                    robot_args_json = json.dumps(robot_args + ["--include", include])
                    stage_task(
                        stage=stage,
                        coverage_tasks=coverage_tasks,
                        group_name=group_name,
                        task_name=f"integration_{include}_{build}_{machine}",
                        build=build,
                        func=run_av_integration_tests,
                        machine=stage_tap_machine(machine, inputs, outputs, build),
                        robot_args_json=robot_args_json,
                    )

            for machine in test_machines:
                stage_task(
                    stage=stage,
                    coverage_tasks=coverage_tasks,
                    group_name=group_name,
                    task_name=f"component_{build}_{machine}",
                    build=build,
                    func=run_av_component_tests,
                    machine=stage_tap_machine(machine, inputs, outputs, build),
                )

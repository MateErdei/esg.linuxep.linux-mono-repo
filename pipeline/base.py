# Copyright 2023 Sophos Limited. All rights reserved.

import json
from typing import Any, Dict, List

import tap.v1 as tap
from tap._pipeline.tasks import ArtisanInput
from dotdict import DotDict

from pipeline.common import (
    unified_artifact,
    get_test_machines,
    get_robot_args,
    TEST_TASK_TIMEOUT_MINUTES,
    get_build_output_for_arch,
    stage_tap_machine,
    stage_task,
    run_robot_tests,
    x86_64,
    get_test_builds,
    get_os_packages,
)


def load_inputs(
    context: tap.PipelineContext, build_output: ArtisanInput, x86_64_build_output: ArtisanInput, build: str
):
    thirdparty_all = unified_artifact(context, "thirdparty.all", "develop", "build")
    openssl = None
    arch = build.split("_")[1]
    if arch == "x64":
        openssl = thirdparty_all / "openssl_3" / "openssl_linux64_gcc11-2glibc2-17"
    elif arch == "arm64":
        openssl = thirdparty_all / "openssl_3" / "openssl_linux_arm64_gcc11-2glibc2-17"

    test_inputs = dict(
        test_scripts=context.artifact.from_folder("./base/testUtils"),
        base_sdds=build_output / f"base/{build}/installer",
        ra_sdds=build_output / f"response_actions/{build}/installer",
        SystemProductTestOutput=build_output / f"base/{build}/SystemProductTestOutput",
        openssl=openssl,
        common_test_libs=context.artifact.from_folder("./common/TA/libs"),
        common_test_utils=context.artifact.from_folder("./common/TA/utils"),
        common_test_robot=context.artifact.from_folder("./common/TA/robot"),
        SupportFiles=context.artifact.from_folder("./base/testUtils/SupportFiles"),
        base_sdds_scripts=context.artifact.from_folder("./base/products/distribution"),
        thininstaller=x86_64_build_output / f"thininstaller/thininstaller",
        PluginAPIMessage_pb2_py=x86_64_build_output / f"base/pluginapimessage_pb2_py",
        sdds3_tools=unified_artifact(context, "em.esg", "develop", f"build/sophlib/linux_{arch}_rel/sdds3_tools"),
    )

    return test_inputs


@tap.timeout(task_timeout=TEST_TASK_TIMEOUT_MINUTES)
def run_base_tests(machine: tap.Machine, robot_args_json: str):
    os_packages = get_os_packages(machine)
    run_robot_tests(
        machine,
        os_packages,
        robot_args=json.loads(robot_args_json),
    )


def stage_base_tests(
    stage: tap.Root,
    context: tap.PipelineContext,
    component: tap.Component,  # pylint: disable=unused-argument
    parameters: DotDict,
    outputs: Dict[str, Any],
    coverage_tasks: List[str],
):
    group_name = "base_tests"

    default_include_tags = "TAP_PARALLEL1,TAP_PARALLEL2,TAP_PARALLEL3,TAP_PARALLEL4,TAP_PARALLEL5,TAP_PARALLEL6"

    with stage.parallel(group_name):
        for build in get_test_builds():
            build_output = get_build_output_for_arch(outputs, build)
            if not build_output:
                continue
            # Base has a special case that it depends on the thininstaller
            if x86_64 not in outputs:
                continue
            inputs = load_inputs(context, build_output, outputs[x86_64], build)
            test_machines = get_test_machines(build, parameters)

            robot_args, single_machine = get_robot_args(parameters)
            includedtags = parameters.include_tags or default_include_tags

            if single_machine:
                for machine in test_machines:

                    robot_args_json = json.dumps(robot_args)
                    stage_task(
                        stage=stage,
                        coverage_tasks=coverage_tasks,
                        group_name=group_name,
                        task_name=f"{build}_{machine}",
                        build=build,
                        func=run_base_tests,
                        machine=stage_tap_machine(machine, inputs, outputs, build),
                        robot_args_json=robot_args_json,
                    )
            else:
                for include in includedtags.split(","):
                    for machine in test_machines:

                        robot_args_json = json.dumps(robot_args + ["--include", include])
                        stage_task(
                            stage=stage,
                            coverage_tasks=coverage_tasks,
                            group_name=group_name,
                            task_name=f"{include}_{build}_{machine}",
                            build=build,
                            func=run_base_tests,
                            machine=stage_tap_machine(machine, inputs, outputs, build),
                            robot_args_json=robot_args_json,
                        )

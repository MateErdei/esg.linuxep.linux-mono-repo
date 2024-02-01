# Copyright 2023 Sophos Limited. All rights reserved.

import json
from typing import Any, Dict, List

import tap.v1 as tap
from tap._pipeline.tasks import ArtisanInput
from dotdict import DotDict

from pipeline.common import (
    get_test_machines,
    get_robot_args,
    run_pytest_tests,
    run_robot_tests,
    TEST_TASK_TIMEOUT_MINUTES,
    get_build_output_for_arch,
    stage_task,
    stage_tap_machine,
    get_test_builds,
    get_os_packages,
)


def get_inputs(context: tap.PipelineContext, build_output: ArtisanInput, build: str):
    test_inputs = dict(
        test_scripts=context.artifact.from_folder("./liveterminal/TA"),
        pytest_scripts=context.artifact.from_component(
            "winep.liveterminal",
            "develop",
            None,
            org="",
            storage="esg-build-tested",
        )
        / "build/sspl-liveterminal/test-scripts",
        common_test_libs=context.artifact.from_folder("./common/TA/libs"),
        common_test_utils=context.artifact.from_folder("./common/TA/utils"),
        common_test_robot=context.artifact.from_folder("./common/TA/robot"),
        SupportFiles=context.artifact.from_folder("./base/testUtils/SupportFiles"),
        tests=context.artifact.from_folder("./base/testUtils/tests"),
        liveresponse=build_output / f"liveterminal/{build}/installer",
        base=build_output / f"base/{build}/installer",
    )

    return test_inputs


@tap.timeout(task_timeout=TEST_TASK_TIMEOUT_MINUTES)
def run_liveterminal_component_tests(machine: tap.Machine):
    os_packages = get_os_packages(machine)
    try:
        run_pytest_tests(machine, ["tests/functional"], os_packages, scripts="pytest_scripts")
    finally:
        machine.output_artifact("/opt/test/dumps", "crash_dumps")


@tap.timeout(task_timeout=TEST_TASK_TIMEOUT_MINUTES)
def run_liveterminal_integration_tests(machine: tap.Machine, robot_args_json: str):
    os_packages = get_os_packages(machine)
    run_robot_tests(machine, os_packages, robot_args=json.loads(robot_args_json))


def stage_liveterminal_tests(
    stage: tap.Root,
    context: tap.PipelineContext,
    component: tap.Component,  # pylint: disable=unused-argument
    parameters: DotDict,
    outputs: Dict[str, Any],
    coverage_tasks: List[str],
):
    group_name = "liveterminal_tests"

    with stage.parallel(group_name):
        for build in get_test_builds():
            build_output = get_build_output_for_arch(outputs, build)
            if not build_output:
                continue
            inputs = get_inputs(context, build_output, build)
            test_machines = get_test_machines(build, parameters)

            robot_args = get_robot_args(parameters)

            for machine in test_machines:
                stage_task(
                    stage=stage,
                    coverage_tasks=coverage_tasks,
                    group_name=group_name,
                    task_name=f"component_{build}_{machine}",
                    build=build,
                    func=run_liveterminal_component_tests,
                    machine=stage_tap_machine(machine, inputs, outputs, build),
                )

                robot_args_json = json.dumps(robot_args)
                stage_task(
                    stage=stage,
                    coverage_tasks=coverage_tasks,
                    group_name=group_name,
                    task_name=f"integration_{build}_{machine}",
                    build=build,
                    func=run_liveterminal_integration_tests,
                    machine=stage_tap_machine(machine, inputs, outputs, build),
                    robot_args_json=robot_args_json,
                )

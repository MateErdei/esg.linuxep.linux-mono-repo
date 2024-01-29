# Copyright 2023 Sophos Limited. All rights reserved.

import json
from typing import Any, Dict, List

import tap.v1 as tap
from tap._pipeline.tasks import ArtisanInput
from dotdict import DotDict

from pipeline.common import (
    get_test_machines,
    get_robot_args,
    stage_tap_machine,
    TEST_TASK_TIMEOUT_MINUTES,
    run_robot_tests,
    get_build_output_for_arch,
    x86_64,
    stage_task,
    get_test_builds,
    get_os_packages,
)


def get_inputs(context: tap.PipelineContext, build_output: ArtisanInput, x86_64_build_output: ArtisanInput, build: str):
    test_inputs = dict(
        test_scripts=context.artifact.from_folder("./eventjournaler/TA"),
        common_test_libs=context.artifact.from_folder("./common/TA/libs"),
        common_test_utils=context.artifact.from_folder("./common/TA/utils"),
        tests=context.artifact.from_folder("./base/testUtils/tests"),
        event_journaler_sdds=build_output / f"eventjournaler/{build}/installer",
        manual_tools=build_output / f"eventjournaler/{build}/manualTools",
        base_sdds=build_output / f"base/{build}/installer",
        PluginAPIMessage_pb2_py=x86_64_build_output / f"base/pluginapimessage_pb2_py"
    )
    test_inputs["SupportFiles/PluginCommunicationTools"] = context.artifact.from_folder('./base/testUtils/SupportFiles/PluginCommunicationTools')
    return test_inputs


@tap.timeout(task_timeout=TEST_TASK_TIMEOUT_MINUTES)
def run_ej_tests(machine: tap.Machine, robot_args_json: str):
    os_packages = get_os_packages(machine)
    run_robot_tests(
        machine,
        os_packages,
        robot_args=json.loads(robot_args_json),
    )


def stage_ej_tests(
    stage: tap.Root,
    context: tap.PipelineContext,
    component: tap.Component,  # pylint: disable=unused-argument
    parameters: DotDict,
    outputs: Dict[str, Any],
    coverage_tasks: List[str],
):
    group_name = "eventjournaler_tests"

    with stage.parallel(group_name):
        for build in get_test_builds():
            build_output = get_build_output_for_arch(outputs, build)
            if not build_output:
                continue
            # Base has a special case that it depends on the thininstaller
            if x86_64 not in outputs:
                continue
            inputs = get_inputs(context, build_output, outputs[x86_64], build)
            test_machines = get_test_machines(build, parameters)
            if test_machines[0] is None:
                continue

            robot_args = get_robot_args(parameters)

            for machine in test_machines:
                robot_args_json = json.dumps(robot_args)
                stage_task(
                    stage=stage,
                    coverage_tasks=coverage_tasks,
                    group_name=group_name,
                    task_name=f"{build}_{machine}",
                    build=build,
                    func=run_ej_tests,
                    machine=stage_tap_machine(machine, inputs, outputs, build),
                    robot_args_json=robot_args_json,
                )

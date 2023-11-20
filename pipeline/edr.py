# Copyright 2023 Sophos Limited. All rights reserved.

import json
from typing import Any, Dict, List

import tap.v1 as tap
from tap._pipeline.tasks import ArtisanInput
from dotdict import DotDict

from pipeline.common import (
    unified_artifact,
    get_build_output_for_arch,
    get_test_machines,
    get_robot_args,
    run_robot_tests,
    run_pytest_tests,
    stage_tap_machine,
    TEST_TASK_TIMEOUT_MINUTES,
    stage_task,
    get_test_builds,
)


def get_inputs(context: tap.PipelineContext, build_output: ArtisanInput, build: str):
    test_inputs = dict(
        test_scripts=context.artifact.from_folder("./edr/TA"),
        SupportFiles=context.artifact.from_folder("./base/testUtils/SupportFiles"),
        tests=context.artifact.from_folder("./base/testUtils/tests"),
        edr_sdds=build_output / f"edr/{build}/installer",
        base_sdds=build_output / f"base/{build}/installer",
        componenttests=build_output / f"edr/{build}/componenttests",
        common_test_libs=context.artifact.from_folder("./common/TA/libs"),
        common_test_utils=context.artifact.from_folder("./common/TA/utils"),
        common_test_robot=context.artifact.from_folder("./common/TA/robot"),
        websocket_server=context.artifact.from_component(
            "winep.liveterminal", "develop", None, org="", storage="esg-build-tested"
        )
        / "build/sspl-liveterminal/test-scripts",
        qp=unified_artifact(context, "em.esg", "develop", "build/scheduled-query-pack-sdds"),
        lp=unified_artifact(context, "em.esg", "develop", "build/endpoint-query-pack"),
    )

    return test_inputs


@tap.timeout(task_timeout=TEST_TASK_TIMEOUT_MINUTES)
def run_edr_component_tests(machine: tap.Machine):
    run_pytest_tests(
        machine, [""], scripts="test_scripts", extra_pytest_args_behind_paths=["--html=/opt/test/results/report.html"]
    )


@tap.timeout(task_timeout=TEST_TASK_TIMEOUT_MINUTES)
def run_edr_integration_tests(machine: tap.Machine, robot_args_json: str):
    run_robot_tests(machine, robot_args=json.loads(robot_args_json))


def stage_edr_tests(
    stage: tap.Root,
    context: tap.PipelineContext,
    component: tap.Component,  # pylint: disable=unused-argument
    parameters: DotDict,
    outputs: Dict[str, Any],
    coverage_tasks: List[str],
):
    group_name = "edr_tests"

    default_include_tags = "TAP_PARALLEL1,TAP_PARALLEL2,TAP_PARALLEL3,TAP_PARALLEL4"

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
                        func=run_edr_integration_tests,
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
                    func=run_edr_component_tests,
                    machine=stage_tap_machine(machine, inputs, outputs, build),
                )

# Copyright 2023 Sophos Limited. All rights reserved.

#TODO Ignore this for now, was present just for testing that the fuzzers build on tap and include the expected output.
#TODO LINUXDAR-7869, tap-pipeline-changes will address this and create the ESG CI job which will run the fuzzers
import tap.v1 as tap
from tap._pipeline.tasks import ArtisanInput

from tap._backend import Input
from dotdict import DotDict

from pipeline.common import (
    # unified_artifact,
    # get_test_machines,
    # get_robot_args,
    # TEST_TASK_TIMEOUT_MINUTES,
    # get_test_builds,
    # get_build_output_for_arch,
    stage_tap_machine,
    stage_task,
    # run_pytest_tests,
    # run_robot_tests,
)

def stage_fuzz_builds(stage: tap.Root, component: tap.Component, fuzz_targets: str, build_template: str, package: str):
    clang_build = stage.artisan_build(name=f"linux_x64_fuzzing",
                                      component=component,
                                      image=build_template,
                                      mode="all_lx64f",
                                      release_package=package)
    print(type(clang_build))
    return clang_build

def stage_fuzzing(stage: tap.Root, context: tap.PipelineContext, component: tap.Component, fuzz_targets: str, build_template: str, package: str):
    fuzz_build = stage_fuzz_builds(stage, component, fuzz_targets, build_template, package)
    # stage_fuzz_tests(stage, context, component, fuzz_targets, fuzz_build)


# def stage_fuzz_tests(
#         stage: tap.Root,
#         context: tap.PipelineContext,
#         component: tap.Component,  # pylint: disable=unused-argument
#         fuzz_targets: str,
#         # parameters: DotDict,
#         build,
#         # coverage_tasks: List[str],
# ):
#     with stage.parallel("fuzz_tests"):
#         if not build:
#             return
#         fuzz_input = build / f"av/linux_x64_clang/fuzz_test_assets",
#         test_machines = get_test_machines(build, parameters)
#
#         robot_args = get_robot_args(parameters)
#         includedtags = parameters.include_tags or default_include_tags
#
#         for include in includedtags.split(","):
#             for machine in test_machines:
#                 robot_args_json = json.dumps(robot_args + ["--include", include])
#                 stage_task(
#                     stage=stage,
#                     coverage_tasks=coverage_tasks,
#                     group_name=group_name,
#                     task_name=f"{include}_{build}_{machine}",
#                     build=build,
#                     func=run_base_tests,
#                     machine=stage_tap_machine(machine, fuzz_input, outputs, build),
#                     robot_args_json=robot_args_json)

# Copyright 2023 Sophos Limited. All rights reserved.

import tap.v1 as tap

from tap._pipeline.api import ArtisanInput
from tap._backend import Input
from dotdict import DotDict

from pipeline.common import (
    get_test_machines,
    stage_tap_machine,
    stage_task,
)

#Binary must finish first, so it provides results and log
FUZZER_JOB_TIMEOUT = 12 * 60 * 60
FUZZER_BINARY_TIMEOUT = 11 * 60 * 60

#For Testing
#FUZZER_JOB_TIMEOUT = 11 * 60
#FUZZER_BINARY_TIMEOUT = 10 * 60

#Inputs
TEST_DIR = "/opt/test/inputs"
FUZZ_TARGET_DIR = f'{TEST_DIR}/fuzz_targets'
FUZZ_SUPPORT_DIR = f'{TEST_DIR}/fuzz_support'

#Additional paths
FUZZ_LOG_DIR = f'{TEST_DIR}/log'
FUZZ_LOG = f'{FUZZ_LOG_DIR}/fuzz-log.txt'
ARTIFACT_OUTPUT_DIR = f'{TEST_DIR}/artifacts'
SETUP_FUZZ_MACHINE = f'{FUZZ_SUPPORT_DIR}/setup_for_fuzzing.sh'
CHECK_FUZZ_OUTPUT = f'{FUZZ_SUPPORT_DIR}/check_fuzzer_output.sh'

COMMAND_BODY = ["2>&1", "-print_final_stats=1", "-timeout=60", "-detect_leaks=1", "-rss_limit_mb=2048",
                    "-max_len=4096", "-use_value_profile=1", f'-max_total_time={FUZZER_BINARY_TIMEOUT}',
                    f'-artifact_prefix={ARTIFACT_OUTPUT_DIR}/', "|", "tee", f'{FUZZ_LOG}']

FUZZER_BUILD = "linux_x64_clang"
FUZZER_TAG = "linux_x64_fuzzing"

@tap.timeout(task_timeout=FUZZER_JOB_TIMEOUT)
@tap.cache(ttl=0)
def run_fuzzer(machine: tap.Machine, fuzz_argument: str):
    try:
        machine.run("bash", f'.{SETUP_FUZZ_MACHINE}')
        machine.run("mkdir", "-p", f'{ARTIFACT_OUTPUT_DIR}')
        machine.run("mkdir", "-p", f'{FUZZ_LOG_DIR}')
        machine.run("chmod", "+x", f'{FUZZ_TARGET_DIR}/{fuzz_argument}')

        machine.run(f'.{FUZZ_TARGET_DIR}/{fuzz_argument}', *COMMAND_BODY, timeout=FUZZER_JOB_TIMEOUT)
    finally:
        machine.output_artifact(f'{FUZZ_LOG_DIR}', "fuzzlog")
        machine.output_artifact(f'{ARTIFACT_OUTPUT_DIR}', "artifacts")
        machine.run("bash", f'.{CHECK_FUZZ_OUTPUT}')


def stage_fuzz_builds(stage: tap.Root, component: tap.Component, build_template: str, package: str) -> ArtisanInput:
    clang_build = stage.artisan_build(name=f"{FUZZER_TAG}",
                                      component=component,
                                      image=build_template,
                                      mode="all_lx64f",
                                      release_package=package)
    return clang_build


def stage_fuzz_tests(
        stage: tap.Root,
        context: tap.PipelineContext,
        parameters: DotDict,
        outputs: ArtisanInput,
):
    group_name = "fuzzer_tests"

    targets_to_fuzz = {
        'av': ['CorcPolicyProcessorFuzzer', 'CorePolicyProcessorFuzzer', 'ProcessControlServerExecutable',
                'threatDetectorClientExecutable'],
        'base': ['ActionRunnerTest', 'SimpleFunctionTests', 'PluginApiTest', 'ManagementAgentApiTest'],
        'edr': ['LiveQueryInputTests', 'LiveQueryTests']
        }


    if not outputs:
        return

    test_machine = get_test_machines(FUZZER_BUILD, parameters)

    for component, fuzzer_object in targets_to_fuzz.items():
        inputs = dict(
            fuzz_targets=outputs / f"{component}/{FUZZER_BUILD}/fuzz_test_assets",
            fuzz_support=context.artifact.from_folder("./common/fuzzer/fuzzer_support")
        )
        for object in fuzzer_object:
            stage_task(
                stage=stage,
                coverage_tasks=[],
                group_name=group_name,
                task_name=f"{FUZZER_TAG}_{component}_{object}",
                build=outputs,
                func=run_fuzzer,
                machine=stage_tap_machine(test_machine[0], inputs, outputs, 'linux_x64_clang'),
                fuzz_argument=object)


def stage_fuzzing(stage: tap.Root, context: tap.PipelineContext, component: tap.Component, parameters: DotDict, build_template: str, package: str):
    fuzz_build = stage_fuzz_builds(stage, component, build_template, package)
    stage_fuzz_tests(stage, context, parameters, fuzz_build)

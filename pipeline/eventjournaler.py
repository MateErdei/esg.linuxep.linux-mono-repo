import os

import requests
import tap.v1 as tap
from tap._pipeline.tasks import ArtisanInput

from pipeline.common import ANALYSIS_MODE, unified_artifact, get_test_machines, COVERAGE_MODE, pip_install, \
    get_robot_args, COVERAGE_TEMPLATE, ROBOT_TEST_TIMEOUT, TASK_TIMEOUT

SYSTEM_TEST_BULLSEYE_JENKINS_JOB_URL = 'https://sspljenkins.eng.sophos/job/SSPL-Plugin-Event-Journaler-bullseye-system-test-coverage/build?token=sspl-linuxdarwin-coverage-token'
# For branch names, remember that slashes are replaced with hyphens:  '/' -> '-'
SYSTEM_TEST_BULLSEYE_CI_BUILD_BRANCH = 'develop'

COVFILE_UNITTEST = '/opt/test/inputs/coverage/sspl-plugin-eventjournaler-unit.cov'
COVFILE_TAPTESTS = '/opt/test/inputs/coverage/sspl-plugin-eventjournaler-tap.cov'
UPLOAD_SCRIPT = '/opt/test/inputs/bullseye_files/uploadResults.sh'
LOGS_DIR = '/opt/test/logs'
RESULTS_DIR = '/opt/test/results'
INPUTS_DIR = '/opt/test/inputs'

NAME = "event_journaler"


def get_inputs(context: tap.PipelineContext, ej_build: ArtisanInput, mode: str):
    test_inputs = None
    if mode == 'release':
        test_inputs = dict(
            test_scripts=context.artifact.from_folder('./eventjournaler/TA'),
            event_journaler_sdds=ej_build / 'eventjournaler/SDDS-COMPONENT',
            manual_tools=ej_build / 'eventjournaler/manualTools',
            base_sdds=ej_build / 'eventjournaler/base/base-sdds',
            fake_management=ej_build / 'eventjournaler/base/fake-management'
        )
    if mode == 'coverage':
        test_inputs = dict(  # TODO check this by setting mode and tap ls
            test_scripts=context.artifact.from_folder('./eventjournaler/TA'),
            event_journaler_sdds=ej_build / 'sspl-plugin-eventjournaler-coverage/SDDS-COMPONENT',
            manual_tools=ej_build / 'sspl-plugin-eventjournaler-coverage/manualTools',
            bullseye_files=context.artifact.from_folder('./eventjournaler/build/bullseye'),
            coverage=ej_build / 'sspl-plugin-eventjournaler-coverage/covfile',
            coverage_unittest=ej_build / 'sspl-plugin-eventjournaler-coverage/unittest-htmlreport',
            base_sdds=ej_build / 'sspl-plugin-eventjournaler-coverage/base/base-sdds',
            fake_management=ej_build / 'sspl-plugin-eventjournaler-coverage/base/fake-management',
            bazel_tools=unified_artifact(context, 'em.esg', 'develop', 'build/bazel-tools')
        )
    return test_inputs


def install_requirements(machine: tap.Machine):
    """ install python lib requirements """
    pip_install(machine, '-r', machine.inputs.test_scripts / 'requirements.txt')


@tap.timeout(task_timeout=TASK_TIMEOUT)
def robot_task(machine: tap.Machine, robot_args: str):
    try:
        install_requirements(machine)
        machine.run(robot_args, 'python3',
                    machine.inputs.test_scripts / 'RobotFramework.py',
                    timeout=ROBOT_TEST_TIMEOUT)
    finally:
        machine.run('python3', machine.inputs.test_scripts / 'move_robot_results.py', timeout=60)
        machine.output_artifact('/opt/test/logs', 'logs')


@tap.timeout(task_timeout=TASK_TIMEOUT)
def coverage_task(machine: tap.Machine, branch: str, robot_args: str):
    try:
        install_requirements(machine)
        tests_dir = str(machine.inputs.test_scripts)

        # upload unit test coverage html results to allegro (and the .cov file which is in unitest_htmldir)
        unitest_htmldir = os.path.join(INPUTS_DIR, "sspl-plugin-eventjournaler-unittest")
        machine.run('mv', str(machine.inputs.coverage_unittest), unitest_htmldir, timeout=30)
        machine.run('bash', '-x', UPLOAD_SCRIPT, environment={
            'UPLOAD_ONLY': 'UPLOAD', 'htmldir': unitest_htmldir, 'COVFILE': COVFILE_UNITTEST, 'COVERAGE_TYPE': 'unit'
        },
                    timeout=120)

        # publish unit test coverage file and results to artifactory results/coverage
        coverage_results_dir = os.path.join(RESULTS_DIR, 'coverage')
        machine.run('rm', '-rf', coverage_results_dir, timeout=60)
        machine.run('mkdir', coverage_results_dir, timeout=60)
        machine.run('cp', "-r", unitest_htmldir, coverage_results_dir, timeout=60)
        machine.run('cp', COVFILE_UNITTEST, coverage_results_dir, timeout=60)

        # run component pytests and integration robot tests with coverage file to get tap coverage
        machine.run('mv', COVFILE_UNITTEST, COVFILE_TAPTESTS, timeout=60)

        # "/tmp/BullseyeCoverageEnv.txt" is a special location that bullseye checks for config values
        # We can set the COVFILE env var here so that all instrumented processes know where it is.
        machine.run("echo", f"COVFILE={COVFILE_TAPTESTS}", ">", "/tmp/BullseyeCoverageEnv.txt", timeout=60)

        # Make sure that any product process can update the cov file, no matter the running user.
        machine.run("chmod", "666", COVFILE_TAPTESTS, timeout=60)

        # run component pytest and tap-tests
        try:
            machine.run(robot_args, 'python3', machine.inputs.test_scripts / 'RobotFramework.py', timeout=ROBOT_TEST_TIMEOUT,
                        environment={'COVFILE': COVFILE_TAPTESTS})
        finally:
            machine.run('python3', machine.inputs.test_scripts / 'move_robot_results.py', timeout=60)

        # generate tap (tap tests + unit tests) coverage html results and upload to allegro (and the .cov file which is in tap_htmldir)
        tap_htmldir = os.path.join(INPUTS_DIR, 'sspl-plugin-eventjournaler-taptest')
        machine.run('mkdir', tap_htmldir, timeout=60)
        machine.run('cp', COVFILE_TAPTESTS, tap_htmldir, timeout=60)
        machine.run('bash', '-x', UPLOAD_SCRIPT,
                    environment={'COVFILE': COVFILE_TAPTESTS, 'BULLSEYE_UPLOAD': '1', 'htmldir': tap_htmldir},
                    timeout=60)

        # publish tap (tap tests + unit tests) html results and coverage file to artifactory
        machine.run('mv', tap_htmldir, coverage_results_dir, timeout=60)
        machine.run('cp', COVFILE_TAPTESTS, coverage_results_dir, timeout=60)

        # trigger system test coverage job on jenkins - this will also upload to allegro
        if branch == SYSTEM_TEST_BULLSEYE_CI_BUILD_BRANCH:
            requests.get(url=SYSTEM_TEST_BULLSEYE_JENKINS_JOB_URL, verify=False)

    finally:
        machine.output_artifact('/opt/test/results', 'results')
        machine.output_artifact('/opt/test/logs', 'logs')


def run_ej_coverage_tests(stage, context, ej_coverage_build, mode, parameters):
    # # Modes where we do not want to run TAP tests.
    # if mode in [ANALYSIS_MODE]:
    #     return

    test_inputs = get_inputs(context, ej_coverage_build, mode)
    robot_args = get_robot_args(parameters)

    with stage.parallel('ej_coverage'):
        stage.task(task_name="coverage",
                   func=coverage_task,
                   machine=tap.Machine(COVERAGE_TEMPLATE, inputs=test_inputs, platform=tap.Platform.Linux),
                   branch=context.branch,
                   robot_args=robot_args)


def run_ej_tests(stage, context, ej_build, mode, parameters):
    # # Modes where we do not want to run TAP tests.
    # if mode in [ANALYSIS_MODE]:
    #     return

    test_inputs = get_inputs(context, ej_build, mode)
    machines = get_test_machines(test_inputs, parameters)
    robot_args = get_robot_args(parameters)

    with stage.parallel('eventjournaler_integration'):
        for template_name, machine in machines:
            stage.task(task_name=template_name, func=robot_task, machine=machine, robot_args=robot_args)

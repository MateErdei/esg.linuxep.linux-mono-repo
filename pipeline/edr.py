import logging
import os

import requests
import tap.v1 as tap
from tap._pipeline.tasks import ArtisanInput

from pipeline.common import ANALYSIS_MODE, unified_artifact, pip_install, get_test_machines, get_robot_args, \
    COVERAGE_TEMPLATE, ROBOT_TEST_TIMEOUT, TASK_TIMEOUT

logger = logging.getLogger(__name__)

SYSTEM_TEST_BULLSEYE_JENKINS_JOB_URL = 'https://sspljenkins.eng.sophos/job/SSPL-EDR-Plugin-bullseye-system-test-coverage/build?token=sspl-linuxdarwin-coverage-token'
# For branch names, remember that slashes are replaced with hyphens:  '/' -> '-'
SYSTEM_TEST_BULLSEYE_CI_BUILD_BRANCH = 'develop'

COVFILE_UNITTEST = '/opt/test/inputs/coverage/sspl-plugin-edr-unit.cov'
COVFILE_TAPTESTS = '/opt/test/inputs/coverage/sspl-plugin-edr-tap.cov'
UPLOAD_SCRIPT = '/opt/test/inputs/bullseye_files/uploadResults.sh'
LOGS_DIR = '/opt/test/logs'
RESULTS_DIR = '/opt/test/results'
INPUTS_DIR = '/opt/test/inputs'

NAME = "edr"


def get_inputs(context: tap.PipelineContext, edr_build: ArtisanInput, mode: str):
    test_inputs = None
    if mode == 'release':
        test_inputs = dict(
            test_scripts=context.artifact.from_folder('./edr/TA'),
            edr_sdds=edr_build / 'edr/SDDS-COMPONENT',
            base_sdds=edr_build / 'edr/base/base-sdds',
            componenttests=edr_build / 'edr/componenttests',
            common_test_libs=context.artifact.from_folder('./common/TA/libs'),
            qp=unified_artifact(context, 'em.esg', 'develop', 'build/scheduled-query-pack-sdds'),
            lp=unified_artifact(context, 'em.esg', 'develop', 'build/endpoint-query-pack')
        )
    if mode == 'coverage':
        test_inputs = dict(
            test_scripts=context.artifact.from_folder('./edr/TA'),
            edr_sdds=edr_build / 'sspl-edr-coverage/SDDS-COMPONENT',
            bullseye_files=context.artifact.from_folder('./edr/build/bullseye'),
            coverage=edr_build / 'sspl-edr-coverage/covfile',
            coverage_unittest=edr_build / 'sspl-edr-coverage/unittest-htmlreport',
            base_sdds=edr_build / 'sspl-edr-coverage/base/base-sdds',
            componenttests=edr_build / 'sspl-edr-coverage/componenttests',
            common_test_libs=context.artifact.from_folder('./common/TA/libs'),
            qp=unified_artifact(context, 'em.esg', 'develop', 'build/scheduled-query-pack-sdds'),
            lp=unified_artifact(context, 'em.esg', 'develop', 'build/endpoint-query-pack'),
            bazel_tools=unified_artifact(context, 'em.esg', 'develop', 'build/bazel-tools')
        )

    return test_inputs


def install_requirements(machine: tap.Machine):
    """ install python lib requirements """
    pip_install(machine, '-r', machine.inputs.test_scripts / 'requirements.txt')
    # try:
    #     machine.run('useradd', 'sophos-spl-user', timeout=5)
    #     machine.run('useradd', 'sophos-spl-local', timeout=5)
    #     machine.run('groupadd', 'sophos-spl-group', timeout=5)
    # except Exception as ex:
    #     # the previous command will fail if user already exists. But this is not an error
    #     logger.warning("On adding user and group: {}".format(ex))


@tap.timeout(task_timeout=TASK_TIMEOUT)
def coverage_task(machine: tap.Machine, branch: str, robot_args: str):
    try:
        install_requirements(machine)
        tests_dir = str(machine.inputs.test_scripts)

        # upload unit test coverage html results to allegro (and the .cov file which is in unitest_htmldir)
        unitest_htmldir = os.path.join(INPUTS_DIR, "sspl-plugin-edr-unittest")
        machine.run('mv', str(machine.inputs.coverage_unittest), unitest_htmldir, timeout=5)
        machine.run('bash', '-x', UPLOAD_SCRIPT,
                    environment={'UPLOAD_ONLY': 'UPLOAD', 'htmldir': unitest_htmldir, 'COVFILE': COVFILE_UNITTEST,
                                 'COVERAGE_TYPE': 'unit'},
                    timeout=240)

        # publish unit test coverage file and results to artifactory results/coverage
        coverage_results_dir = os.path.join(RESULTS_DIR, 'coverage')
        machine.run('rm', '-rf', coverage_results_dir, timeout=60)
        machine.run('mkdir', '-p', coverage_results_dir, timeout=5)
        machine.run('cp', "-r", unitest_htmldir, coverage_results_dir, timeout=120)
        machine.run('cp', COVFILE_UNITTEST, coverage_results_dir, timeout=60)

        # run component pytests and integration robot tests with coverage file to get tap coverage
        machine.run('mv', COVFILE_UNITTEST, COVFILE_TAPTESTS, timeout=5)

        # "/tmp/BullseyeCoverageEnv.txt" is a special location that bullseye checks for config values
        # We can set the COVFILE env var here so that all instrumented processes know where it is.
        machine.run("echo", f"COVFILE={COVFILE_TAPTESTS}", ">", "/tmp/BullseyeCoverageEnv.txt", timeout=5)

        # Make sure that any product process can update the cov file, no matter the running user.
        machine.run("chmod", "666", COVFILE_TAPTESTS, timeout=5)

        # run component pytest and tap-tests
        args = ['python3', '-u', '-m', 'pytest', tests_dir, '--html=/opt/test/results/report.html']
        machine.run(*args, environment={'COVFILE': COVFILE_TAPTESTS}, timeout=1200)
        try:
            machine.run(robot_args, 'python3', machine.inputs.test_scripts / 'RobotFramework.py',
                        timeout=ROBOT_TEST_TIMEOUT,
                        environment={'COVFILE': COVFILE_TAPTESTS})
        finally:
            machine.run('python3', machine.inputs.test_scripts / 'move_robot_results.py', timeout=60)

        # generate tap (tap tests + unit tests) coverage html results and upload to allegro
        # (and the .cov file which is in tap_htmldir)
        tap_htmldir = os.path.join(INPUTS_DIR, 'sspl-plugin-edr-taptest')
        machine.run('mkdir', tap_htmldir, timeout=5)
        machine.run('cp', COVFILE_TAPTESTS, tap_htmldir, timeout=10)
        machine.run('bash', '-x', UPLOAD_SCRIPT,
                    environment={'COVFILE': COVFILE_TAPTESTS, 'BULLSEYE_UPLOAD': '1', 'htmldir': tap_htmldir},
                    timeout=120)

        # publish tap (tap tests + unit tests) html results and coverage file to artifactory
        machine.run('mv', tap_htmldir, coverage_results_dir, timeout=5)
        machine.run('cp', COVFILE_TAPTESTS, coverage_results_dir, timeout=60)

        # trigger system test coverage job on jenkins - this will also upload to allegro
        if branch == SYSTEM_TEST_BULLSEYE_CI_BUILD_BRANCH:
            requests.get(url=SYSTEM_TEST_BULLSEYE_JENKINS_JOB_URL, verify=False)

    finally:
        machine.output_artifact('/opt/test/logs', 'logs')
        machine.output_artifact('/opt/test/coredumps', 'coredumps', raise_on_failure=False)


@tap.timeout(task_timeout=TASK_TIMEOUT)
def robot_task(machine: tap.Machine, robot_args: str, include_tag: str):
    default_exclude_tags = ["TESTFAILURE"]

    print(f"test scripts: {machine.inputs.test_scripts}")
    print(f"robot_args: {robot_args}")
    print(f"include_tag: {include_tag}")
    try:
        install_requirements(machine)

        if include_tag:
            machine.run(*robot_args.split(), 'python3', machine.inputs.test_scripts / 'RobotFramework.py',
                        '--include', include_tag,
                        '--exclude', *default_exclude_tags,
                        timeout=ROBOT_TEST_TIMEOUT)
        else:
            machine.run(*robot_args.split(),
                        python(machine),
                        machine.inputs.test_scripts / 'RobotFramework.py',
                        timeout=ROBOT_TEST_TIMEOUT)

    finally:
        machine.run('python3', machine.inputs.test_scripts / 'move_robot_results.py', timeout=60)
        machine.output_artifact('/opt/test/logs', 'logs')
        machine.output_artifact('/opt/test/coredumps', 'coredumps', raise_on_failure=False)



def pytest_task(machine: tap.Machine):
    try:
        install_requirements(machine)
        tests_dir = str(machine.inputs.test_scripts)

        # To run the pytest with additional verbosity add following to arguments
        # '-o', 'log_cli=true'
        args = ['python3', '-u', '-m', 'pytest', tests_dir, '--html=/opt/test/results/report.html']
        machine.run(*args, timeout=1200)
        machine.run('ls', '/opt/test/logs', timeout=10)
    finally:
        machine.output_artifact('/opt/test/results', 'results')
        machine.output_artifact('/opt/test/logs', 'logs')


def run_edr_coverage_tests(stage, context, edr_coverage_build, mode, parameters):
    robot_args = get_robot_args(parameters)
    edr_test_inputs = get_inputs(context, edr_coverage_build, mode)
    with stage.parallel('edr_coverage'):
        stage.task(task_name="coverage",
                   func=coverage_task,
                   machine=tap.Machine(COVERAGE_TEMPLATE, inputs=edr_test_inputs, platform=tap.Platform.Linux),
                   branch=context.branch,
                   robot_args=robot_args)


def run_edr_tests(stage, context, edr_build, mode, parameters):
    #exclude tags are in robot_task
    default_include_tags = "TAP_PARALLEL1,TAP_PARALLEL2,TAP_PARALLEL3"

    test_inputs = get_inputs(context, edr_build, mode)
    test_machines = get_test_machines(test_inputs, parameters)
    robot_args = get_robot_args(parameters)

    with stage.parallel('edr_test'):
        with stage.parallel('integration'):
            if robot_args:
                for template_name, machine in test_machines:
                    print("machine", robot_args, template_name, machine)
                    stage.task(task_name=template_name,
                               func=robot_task,
                               machine=machine,
                               robot_args=robot_args,
                               include_tag="")

            else:
                includedtags = parameters.include_tags or default_include_tags
                for include in includedtags.split(","):
                    with stage.parallel(include):
                        for template_name, machine in test_machines:
                            print("machine", include, template_name, machine)
                            stage.task(task_name=template_name,
                                       func=robot_task,
                                       machine=machine,
                                       robot_args="",
                                       include_tag=include)

        with stage.parallel('component'):
            for template_name, machine in get_test_machines(test_inputs, parameters):
                stage.task(task_name=template_name, func=pytest_task, machine=machine)

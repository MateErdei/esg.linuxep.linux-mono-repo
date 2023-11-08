# Copyright 2023 Sophos Limited. All rights reserved.
import os

import requests
import tap.v1 as tap
from tap._pipeline.tasks import ArtisanInput

from pipeline.common import get_test_machines, pip_install, python, get_os_packages, \
    get_robot_args, COVERAGE_TEMPLATE

SYSTEM_TEST_BULLSEYE_JENKINS_JOB_URL = 'https://sspljenkins.eng.sophos/job/SSPL-Plugin-Event-Journaler-bullseye-system-test-coverage/build?token=sspl-linuxdarwin-coverage-token'
# For branch names, remember that slashes are replaced with hyphens:  '/' -> '-'
SYSTEM_TEST_BULLSEYE_CI_BUILD_BRANCH = 'develop'

COVFILE_UNITTEST = '/opt/test/inputs/coverage/liveterminal_unittests.cov'
COVFILE_TAPTESTS = '/opt/test/inputs/coverage/sspl-plugin-liveterminal-tap.cov'
UPLOAD_SCRIPT = '/opt/test/inputs/bullseye_files/uploadResults.sh'
LOGS_DIR = '/opt/test/logs'
RESULTS_DIR = '/opt/test/results'
INPUTS_DIR = '/opt/test/inputs'



def get_inputs(context: tap.PipelineContext, liveterminal_build: ArtisanInput, mode: str, arch: str = "x64"):
    test_inputs = None
    if liveterminal_build is None:
        return None

    if mode == 'coverage':
        test_inputs = dict(
            test_scripts=context.artifact.from_folder('./liveterminal/TA'),
            pytest_scripts=context.artifact.from_component("winep.liveterminal", "develop", None, org="",
                                                            storage="esg-build-tested") / "build/sspl-liveterminal/test-scripts",
            common_test_libs=context.artifact.from_folder('./common/TA/libs'),
            common_test_robot=context.artifact.from_folder('./common/TA/robot'),
            SupportFiles=context.artifact.from_folder('./base/testUtils/SupportFiles'),
            tests=context.artifact.from_folder('./base/testUtils/tests'),
            liveresponse=liveterminal_build / "sspl-liveterminal-coverage/sdds",
            base=liveterminal_build / "sspl-liveterminal-coverage/base_sdds",
            coverage=liveterminal_build /'sspl-liveterminal-coverage/covfile',
            coverage_unittest=liveterminal_build /'sspl-liveterminal-coverage/unittest-htmlreport',
            bullseye_files=context.artifact.from_folder('./base/build/bullseye'),
        )

    if mode == 'release':
        test_inputs = dict(
            test_scripts=context.artifact.from_folder('./liveterminal/TA'),
            pytest_scripts=context.artifact.from_component("winep.liveterminal", "develop", "20231103050000-24b504abf94ec0a99e9180998f9cc56c3edcde4f-hKKQEI", org="",
                                                            storage="esg-build-tested") / "build/sspl-liveterminal/test-scripts",
            common_test_libs=context.artifact.from_folder('./common/TA/libs'),
            common_test_robot=context.artifact.from_folder('./common/TA/robot'),
            SupportFiles=context.artifact.from_folder('./base/testUtils/SupportFiles'),
            tests=context.artifact.from_folder('./base/testUtils/tests'),
            liveresponse=liveterminal_build / f"liveterminal/linux_{arch}_rel/installer",
            base=liveterminal_build / f"base/linux_{arch}_rel/installer",
        )

    return test_inputs


def install_requirements(machine: tap.Machine):
    """ install python lib requirements """
    machine.run("which", python(machine))
    machine.run(python(machine), "-V")

    try:
        # Check if python2 is python on this machine
        machine.run("which", "python")
        machine.run("python", "-V")
    except Exception:
        pass

    try:
        pip_install(machine, "-r", machine.inputs.test_scripts / "requirements.txt")

        os_packages = get_os_packages(machine)
        install_command = ["bash", machine.inputs.SupportFiles / "install_os_packages.sh"] + os_packages
        machine.run(*install_command)
    except Exception as ex:
        # the previous command will fail if user already exists. But this is not an error
        print(f"On adding installing requirements: {ex}")


def robot_task(machine: tap.Machine, robot_args: str, machine_name: str):
    arch, platform = machine_name.split("_")
    default_exclude_tags = [f"EXCLUDE_{platform.upper()}", f"EXCLUDE_{arch.upper()}"]
    try:
        install_requirements(machine)
        machine.run(robot_args, 'python3', machine.inputs.test_scripts / 'RobotFramework.py',
                    '--exclude', *default_exclude_tags, timeout=3600)
    finally:
        machine.run('python3', machine.inputs.test_scripts / 'move_robot_results.py')
        machine.output_artifact('/opt/test/logs', 'logs')

def test_args(python, folder):
    args = [python, "-u", "-m", "pytest", folder]
    args += ["--durations=0", "-v", "-s"]

    # Set TAP_PARAMETER_TEST_PATTERN environment variable to a pytest filter to narrow scope when writing tests.
    test_filter = os.getenv("TAP_PARAMETER_TEST_PATTERN")
    if bool(test_filter):
        args += ["--suppress-no-test-exit-code", "-k", test_filter]

    return args


def run_pytests(machine: tap.Machine):

    results_dir = "/opt/test/results"
    dumps_dir = "/opt/test/dumps"
    logs_dir = "/opt/test/logs"
    python = "python3"
    pip = "pip3"

    try:
        install_requirements(machine)


        env = {
            "COVFILE": os.path.join(results_dir, f"liveterminal_functional_{machine.template}.cov"),
            "COVERR": os.path.join(results_dir, "bullseye.err"),
            "BRANCH_NAME": os.environ.get("BRANCH_NAME", "unknown-branch"),
        }

        if machine.inputs.get("coverage", None):
            machine.run(
                python,
                machine.inputs.pytest_scripts / "utils" / "bullseye.py",
                machine.inputs["coverage"] / "covfile" / "liveterminal_empty.cov",
                environment=env,
                )


        #TODO LINUXDAR-8169 remove this section
        exclude_proxy_test =['ubuntu1804_x64_aws_server_en_us',
                             'ubuntu2004_x64_aws_server_en_us',
                             'debian11_x64_aws_server_en_us',
                             'debian10_x64_aws_server_en_us',
                             'amzlinux2023_x64_server_en_us',
                             'amzlinux2_x64_server_en_us',

                             'ubuntu1804_arm64_server_en_us',
                             'ubuntu2004_arm64_server_en_us',
                             'debian11_arm64_server_en_us',
                             'debian10_arm64_server_en_us',
                             'amzlinux2023_arm64_server_en_us',
                             'amzlinux2_arm64_server_en_us'
                            ]

        if machine.template in exclude_proxy_test:
            machine.run("rm", "-rf", machine.inputs.pytest_scripts / "tests/functional/test_proxy_connection.py")
        args = test_args(python, machine.inputs.pytest_scripts / "tests/functional")
        machine.run(*args, timeout=3600, environment=env)

    finally:
        machine.output_artifact(logs_dir, "logs")
        machine.output_artifact(results_dir, "results")
        machine.output_artifact(dumps_dir, "crash_dumps")

def coverage_task(machine: tap.Machine, branch: str, robot_args: str):
    try:
        install_requirements(machine)
        tests_dir = str(machine.inputs.test_scripts)

        # upload unit test coverage html results to allegro (and the .cov file which is in unitest_htmldir)
        unitest_htmldir = os.path.join(INPUTS_DIR, "sspl-plugin-liveterminal-unittest")
        machine.run('mv', str(machine.inputs.coverage_unittest), unitest_htmldir)
        machine.run('bash', '-x', UPLOAD_SCRIPT, environment={
            'UPLOAD_ONLY': 'UPLOAD', 'htmldir': unitest_htmldir, 'COVFILE': COVFILE_UNITTEST, 'COVERAGE_TYPE': 'unit'
        })

        # publish unit test coverage file and results to artifactory results/coverage
        coverage_results_dir = os.path.join(RESULTS_DIR, 'coverage')
        machine.run('rm', '-rf', coverage_results_dir)
        machine.run('mkdir', coverage_results_dir)
        machine.run('cp', "-r", unitest_htmldir, coverage_results_dir)
        machine.run('cp', COVFILE_UNITTEST, coverage_results_dir)

        # run component pytests and integration robot tests with coverage file to get tap coverage
        machine.run('mv', COVFILE_UNITTEST, COVFILE_TAPTESTS)

        # "/tmp/BullseyeCoverageEnv.txt" is a special location that bullseye checks for config values
        # We can set the COVFILE env var here so that all instrumented processes know where it is.
        machine.run("echo", f"COVFILE={COVFILE_TAPTESTS}", ">", "/tmp/BullseyeCoverageEnv.txt")

        # Make sure that any product process can update the cov file, no matter the running user.
        machine.run("chmod", "666", COVFILE_TAPTESTS)
        env = {
            "COVFILE": COVFILE_TAPTESTS,
            "BRANCH_NAME": os.environ["BRANCH_NAME"],
        }

        # run component pytest and tap-tests

        args = test_args('python3', machine.inputs.pytest_scripts / "tests/functional")
        machine.run(*args, timeout=3600, environment=env)


        # generate tap (tap tests + unit tests) coverage html results and upload to allegro (and the .cov file which is in tap_htmldir)
        tap_htmldir = os.path.join(INPUTS_DIR, 'sspl-plugin-liveterminal-taptest')
        machine.run('mkdir', tap_htmldir)
        machine.run('cp', COVFILE_TAPTESTS, tap_htmldir)
        machine.run('bash', '-x', UPLOAD_SCRIPT,
                    environment={'COVFILE': COVFILE_TAPTESTS, 'BULLSEYE_UPLOAD': '1', 'htmldir': tap_htmldir})

        # publish tap (tap tests + unit tests) html results and coverage file to artifactory
        machine.run('mv', tap_htmldir, coverage_results_dir)
        machine.run('cp', COVFILE_TAPTESTS, coverage_results_dir)

        # trigger system test coverage job on jenkins - this will also upload to allegro
        if branch == SYSTEM_TEST_BULLSEYE_CI_BUILD_BRANCH:
            requests.get(url=SYSTEM_TEST_BULLSEYE_JENKINS_JOB_URL, verify=False)

    finally:
        machine.output_artifact('/opt/test/results', 'results')
        machine.output_artifact('/opt/test/logs', 'logs')


def run_liveterminal_coverage_tests(stage, context, liveterminal_coverage_build, mode, parameters):
    # # Modes where we do not want to run TAP tests.
    # if mode in [ANALYSIS_MODE]:
    #     return

    test_inputs = get_inputs(context, liveterminal_coverage_build, mode)
    robot_args = get_robot_args(parameters)

    with stage.parallel('liveterminal_coverage'):
        stage.task(task_name="coverage",
                   func=coverage_task,
                   machine=tap.Machine(COVERAGE_TEMPLATE, inputs=test_inputs, platform=tap.Platform.Linux),
                   branch=context.branch,
                   robot_args=robot_args)


def run_liveterminal_tests(stage, context, builds, mode, parameters):
    # # Modes where we do not want to run TAP tests.
    # if mode in [ANALYSIS_MODE]:
    #     return

    test_inputs = {
        "x64": get_inputs(context, builds["x86_64"], mode, "x64"),
        "arm64": get_inputs(context, builds["arm64"], mode, "arm64")
    }
    machines = get_test_machines(test_inputs, parameters)
    robot_args = get_robot_args(parameters)

    with stage.parallel('liveterminal_pytests'):
        for template_name, machine in machines:
            stage.task(task_name=template_name, func=run_pytests, machine=machine)

    with stage.parallel('liveterminal_integration'):
        for template_name, machine in machines:
            stage.task(task_name=template_name, func=robot_task, machine=machine, robot_args=robot_args,
                       machine_name=template_name)

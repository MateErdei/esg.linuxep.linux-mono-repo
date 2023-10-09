# Copyright 2023 Sophos Limited. All rights reserved.
import os
import requests

import tap.v1 as tap
from tap._pipeline.tasks import ArtisanInput

from pipeline.common import unified_artifact, package_install, get_test_machines, pip_install, get_suffix, \
    COVERAGE_MODE, COVERAGE_TEMPLATE, get_robot_args, ROBOT_TEST_TIMEOUT, TASK_TIMEOUT, get_os_packages, python

PACKAGE_PATH = "./base/build/release-package.xml"
INPUTS_DIR = '/opt/test/inputs'
COVFILE_UNITTEST = os.path.join(INPUTS_DIR, 'coverage/sspl-base-unittest.cov')
COVFILE_TAPTESTS = os.path.join(INPUTS_DIR, 'coverage/sspl-base-taptests.cov')
BULLSEYE_SCRIPT_DIR = os.path.join(INPUTS_DIR, 'bullseye_files')
UPLOAD_SCRIPT = os.path.join(BULLSEYE_SCRIPT_DIR, 'uploadResults.sh')
UPLOAD_ROBOT_LOG_SCRIPT = os.path.join(BULLSEYE_SCRIPT_DIR, 'uploadRobotLog.sh')
RESULTS_DIR = '/opt/test/results'
BRANCH_NAME = "develop"

# Coverage
SYSTEM_TEST_BULLSEYE_JENKINS_JOB_URL = 'https://sspljenkins.eng.sophos/job/SSPL-Base-bullseye-system-test-coverage/build?token=sspl-linuxdarwin-coverage-token'
# For branch names, remember that slashes are replaced with hyphens:  '/' -> '-'
SYSTEM_TEST_BULLSEYE_CI_BUILD_BRANCH = 'develop'


def get_base_test_inputs(context: tap.PipelineContext, base_build: ArtisanInput, mode: str):
    openssl = unified_artifact(context, "thirdparty.all", "develop",
                               "build/openssl_3/openssl_linux64_gcc11-2glibc2-17")

    test_inputs = None
    if mode == 'release':
        test_inputs = dict(
            test_scripts=context.artifact.from_folder('./base/testUtils'),
            base_sdds=base_build / "base/linux_x64_rel/installer",
            ra_sdds=base_build / "response_actions/linux_x64_rel/installer",
            system_test=base_build / "base/linux_x64_rel/system_test",
            openssl=openssl,
            websocket_server=context.artifact.from_component('liveterminal', 'prod',
                                                             '1-0-267/219514') / 'websocket_server',
            bullseye_files=context.artifact.from_folder('./base/build/bullseye'),  # used for robot upload
            common_test_libs=context.artifact.from_folder('./common/TA/libs'),
            thininstaller=base_build / "thininstaller/linux_x64_rel/thininstaller",
            sdds3_tools=unified_artifact(context, 'em.esg', 'develop', 'build/sophlib/linux_x64_rel/sdds3_tools')
        )
    if mode == 'debug':
        test_inputs = dict(
            test_scripts=context.artifact.from_folder('./base/testUtils'),
            base_sdds=base_build / "base/linux_x64_dbg/installer",
            ra_sdds=base_build / "response_actions/linux_x64_dbg/installer",
            system_test=base_build / "base/linux_x64_dbg/system_test",
            openssl=openssl,
            websocket_server=context.artifact.from_component('liveterminal', 'prod',
                                                             '1-0-267/219514') / 'websocket_server',
            bullseye_files=context.artifact.from_folder('./base/build/bullseye'),  # used for robot upload
            common_test_libs=context.artifact.from_folder('./common/TA/libs'),
            thininstaller=base_build / "thininstaller/linux_x64_rel/thininstaller",
            sdds3_tools=unified_artifact(context, 'em.esg', 'develop', 'build/sophlib/linux_x64_rel/sdds3_tools')
        )
    if mode == 'coverage':
        test_inputs = dict(
            test_scripts=context.artifact.from_folder('./base/testUtils'),
            base_sdds=base_build / 'sspl-base-coverage/SDDS-COMPONENT',
            ra_sdds=base_build / 'sspl-base-coverage/RA-SDDS-COMPONENT',
            system_test=base_build / 'sspl-base-coverage/system_test',
            openssl=openssl,
            websocket_server=context.artifact.from_component('liveterminal', 'prod',
                                                             '1-0-267/219514') / 'websocket_server',
            bullseye_files=context.artifact.from_folder('./base/build/bullseye'),
            common_test_libs=context.artifact.from_folder('./common/TA/libs'),
            coverage=base_build / 'sspl-base-coverage/covfile',
            coverage_unittest=base_build / 'sspl-base-coverage/unittest-htmlreport',
            bazel_tools=unified_artifact(context, 'em.esg', 'develop', 'build/bazel-tools')
        )
    return test_inputs


@tap.timeout(task_timeout=TASK_TIMEOUT)
def robot_task(machine: tap.Machine, branch_name: str, robot_args: str, include_tag: str, machine_name: str):
    default_exclude_tags = ["CENTRAL", "MANUAL", "TESTFAILURE", "FUZZ", "SYSTEMPRODUCTTESTINPUT",
                            "EXCLUDE_BAZEL", f"EXCLUDE_{machine_name.upper()}"]

    machine_full_name = machine.template
    print(f"test scripts: {machine.inputs.test_scripts}")
    print(f"robot_args: {robot_args}")
    print(f"include_tag: {include_tag}")
    try:
        install_requirements(machine)

        if include_tag:
            machine.run(python(machine), machine.inputs.test_scripts / 'RobotFramework.py',
                        '--include', include_tag,
                        '--exclude', *default_exclude_tags,
                        timeout=ROBOT_TEST_TIMEOUT)
        else:
            machine.run(*robot_args.split(), python(machine), machine.inputs.test_scripts / 'RobotFramework.py'
                        , timeout=ROBOT_TEST_TIMEOUT)


    finally:
        machine.run('python3', machine.inputs.test_scripts / 'move_robot_results.py', timeout=60)
        machine.output_artifact('/opt/test/logs', 'logs')
        machine.output_artifact('/opt/test/results', 'results')

        if include_tag:
            machine.run('bash', UPLOAD_ROBOT_LOG_SCRIPT, "/opt/test/results/log.html",
                        branch_name + "/base" + get_suffix(
                            branch_name) + "_" + machine_full_name + "_" + include_tag + "-log.html",
                        timeout=120)
        elif robot_args:
            machine.run('bash', UPLOAD_ROBOT_LOG_SCRIPT, "/opt/test/results/log.html",
                        branch_name + "/base" + get_suffix(
                            branch_name) + "_" + machine_full_name + "_" + robot_args + "-log.html",
                        timeout=120)


def install_requirements(machine: tap.Machine):
    """ install python lib requirements """

    try:
        # Check if python2 is python on this machine
        machine.run("which", python(machine), timeout=5)
        machine.run(python(machine), "-V", timeout=5)
        machine.run("which", "python", timeout=5)
        machine.run("python", "-V", timeout=5)
    except Exception:
        pass

    # TODO LINUXDAR-5855 remove this section when we stop signing with sha1
    try:
        if machine.template == "centos9stream_x64_aws_server_en_us" or machine.template == "rhel91_x64_aws_server_en_us":
            machine.run("update-crypto-policies", "--set", "LEGACY", timeout=30)
    except Exception as ex:
        print(f"On updating openssl policy: {ex}")

    try:
        pip_install(machine, "-r", machine.inputs.test_scripts / "requirements.txt")
        os_packages = get_os_packages(machine)
        install_command = ["bash", machine.inputs.test_scripts / "SupportFiles/install_os_packages.sh"] + os_packages
        machine.run(*install_command, timeout=600)
    except Exception as ex:
        # the previous command will fail if user already exists. But this is not an error
        print(f"On adding installing requirements: {ex}")


def pip(machine: tap.Machine):
    return "pip3"

@tap.timeout(task_timeout=TASK_TIMEOUT)
def coverage_task(machine: tap.Machine, branch: str, robot_args: str):
    try:
        install_requirements(machine)

        # upload unit test coverage-html and the unit-tests cov file to allegro
        unitest_htmldir = os.path.join(INPUTS_DIR, "sspl-base-unittest")
        machine.run('mv', str(machine.inputs.coverage_unittest), unitest_htmldir, timeout=5)
        machine.run('cp', COVFILE_UNITTEST, unitest_htmldir, timeout=5)
        machine.run('bash', '-x', UPLOAD_SCRIPT,
                    environment={'UPLOAD_ONLY': 'UPLOAD', 'htmldir': unitest_htmldir, 'COVERAGE_TYPE': 'unit',
                                 'COVFILE': COVFILE_UNITTEST},
                    timeout=120)

        # publish unit test coverage file and results to artifactory results/coverage
        coverage_results_dir = os.path.join(RESULTS_DIR, 'coverage')
        machine.run('rm', '-rf', coverage_results_dir, timeout=120)
        machine.run('mkdir', coverage_results_dir, timeout=5)
        machine.run('cp', "-r", unitest_htmldir, coverage_results_dir, timeout=120)
        machine.run('cp', COVFILE_UNITTEST, coverage_results_dir, timeout=10)

        # run component pytests and integration robot tests with coverage file to get combined coverage
        machine.run('mv', COVFILE_UNITTEST, COVFILE_TAPTESTS, timeout=5)

        # "/tmp/BullseyeCoverageEnv.txt" is a special location that bullseye checks for config values
        # We can set the COVFILE env var here so that all instrumented processes know where it is.
        machine.run("echo", f"COVFILE={COVFILE_TAPTESTS}", ">", "/tmp/BullseyeCoverageEnv.txt",
                    timeout=5)

        # Make sure that any product process can update the cov file, no matter the running user.
        machine.run("chmod", "666", COVFILE_TAPTESTS,
                    timeout=5)

        # run component pytest -- these are disabled
        # run tap-tests
        try:
            machine.run(robot_args, 'python3', machine.inputs.test_scripts / 'RobotFramework.py',
                        timeout=ROBOT_TEST_TIMEOUT,
                        environment={'COVFILE': COVFILE_TAPTESTS})
        finally:
            machine.run('python3', machine.inputs.test_scripts / 'move_robot_results.py',
                        timeout=5)

        # generate tap (tap tests + unit tests) coverage html results and upload to allegro
        # (and the .cov file which is in tap_htmldir)
        tap_htmldir = os.path.join(INPUTS_DIR, 'sspl-base-taptest')
        machine.run('mkdir', "-p", tap_htmldir,
                    timeout=5)
        machine.run('cp', COVFILE_TAPTESTS, tap_htmldir,
                    timeout=60)
        upload_results = "0"
        # Don't upload results to allegro unless we're on develop, can be overridden.
        if branch == SYSTEM_TEST_BULLSEYE_CI_BUILD_BRANCH:
            upload_results = "1"

        machine.run('bash', '-x', UPLOAD_SCRIPT,
                    environment={'COVFILE': COVFILE_TAPTESTS, 'BULLSEYE_UPLOAD': upload_results,
                                 'htmldir': tap_htmldir},
                    timeout=120)

        # publish tap (tap tests + unit tests) html results and coverage file to artifactory
        machine.run('mv', tap_htmldir, coverage_results_dir,
                    timeout=5)
        machine.run('cp', COVFILE_TAPTESTS, coverage_results_dir,
                    timeout=120)
        if branch == SYSTEM_TEST_BULLSEYE_CI_BUILD_BRANCH:
            # start systemtest coverage in jenkins (these include tap-tests)
            requests.get(url=SYSTEM_TEST_BULLSEYE_JENKINS_JOB_URL, verify=False)

    finally:
        machine.output_artifact('/opt/test/results', 'results')
        machine.output_artifact('/opt/test/logs', 'logs')


def run_base_coverage_tests(stage, context, base_coverage_build, mode, parameters):
    base_test_inputs = get_base_test_inputs(context, base_coverage_build, base_coverage_build, mode)
    robot_args = get_robot_args(parameters)
    with stage.parallel('base_coverage'):
        # if mode == COVERAGE_MODE:
        stage.task(task_name="coverage",
                   func=coverage_task,
                   machine=tap.Machine(COVERAGE_TEMPLATE, inputs=base_test_inputs, platform=tap.Platform.Linux),
                   branch=context.branch,
                   robot_args=robot_args)


def run_base_tests(stage, context, base_build, mode, parameters):
    # exclude tags are in robot_task
    default_include_tags = "TAP_PARALLEL1,TAP_PARALLEL2,TAP_PARALLEL3,TAP_PARALLEL4,TAP_PARALLEL5,TAP_PARALLEL6"

    base_test_inputs = get_base_test_inputs(context, base_build, mode)
    base_test_machines = get_test_machines(base_test_inputs, parameters)
    robot_args = get_robot_args(parameters)

    with stage.parallel('base_integration'):
        if robot_args:
            for template_name, machine in base_test_machines:
                print("machine", robot_args, template_name, machine)
                stage.task(task_name=template_name,
                           func=robot_task,
                           machine=machine,
                           branch_name=context.branch,
                           robot_args=robot_args,
                           include_tag="",
                           machine_name=template_name)
        else:
            includedtags = parameters.include_tags or default_include_tags

            for include in includedtags.split(","):
                with stage.parallel(include):
                    for template_name, machine in base_test_machines:
                        print("machine", include, template_name, machine)
                        stage.task(task_name=template_name,
                                   func=robot_task,
                                   machine=machine,
                                   branch_name=context.branch,
                                   robot_args="",
                                   include_tag=include,
                                   machine_name=template_name)

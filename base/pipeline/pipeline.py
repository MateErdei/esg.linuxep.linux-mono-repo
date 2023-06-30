#!/usr/bin/env python3
# Copyright 2020-2023 Sophos Limited. All rights reserved.
import logging
import os
import sys
import time
import tap.v1 as tap
from tap._pipeline.tasks import ArtisanInput

import requests


SYSTEM_TEST_BULLSEYE_JENKINS_JOB_URL = 'https://sspljenkins.eng.sophos/job/SSPL-Base-bullseye-system-test-coverage/build?token=sspl-linuxdarwin-coverage-token'
# For branch names, remember that slashes are replaced with hyphens:  '/' -> '-'
SYSTEM_TEST_BULLSEYE_CI_BUILD_BRANCH = 'develop'

INPUTS_DIR = '/opt/test/inputs'
COVFILE_UNITTEST = os.path.join(INPUTS_DIR, 'coverage/sspl-base-unittest.cov')
COVFILE_TAPTESTS = os.path.join(INPUTS_DIR, 'coverage/sspl-base-taptests.cov')
BULLSEYE_SCRIPT_DIR = os.path.join(INPUTS_DIR, 'bullseye_files')
UPLOAD_SCRIPT = os.path.join(BULLSEYE_SCRIPT_DIR, 'uploadResults.sh')
UPLOAD_ROBOT_LOG_SCRIPT = os.path.join(BULLSEYE_SCRIPT_DIR, 'uploadRobotLog.sh')

RESULTS_DIR = '/opt/test/results'

COMPONENT = 'sspl_base'
BUILD_TEMPLATE = 'centos79_x64_build_20230202'
BUILD_TEMPLATE_BAZEL = 'centos79_x64_bazel_20230512'
RELEASE_PKG = './build/release-package.xml'
BAZEL_RELEASE_PKG = './build/release-package-bazel.xml'
RELEASE_MODE = 'release'
DEBUG_MODE = 'debug'
ANALYSIS_MODE = 'analysis'
COVERAGE_MODE = 'coverage'
NINE_NINE_NINE_MODE = '999'
ZERO_SIX_ZERO_MODE = '060'

BRANCH_NAME = "develop"

logger = logging.getLogger(__name__)


# Extract base version from release package xml file to save hard-coding it twice.
def get_base_version():
    import xml.dom.minidom
    release_pkg_name = "release-package.xml"
    package_path = os.path.join("./build/", release_pkg_name)
    if os.path.isfile(package_path):
        dom = xml.dom.minidom.parseString(open(package_path).read())
        package = dom.getElementsByTagName("package")[0]
        version = package.getAttribute("version")
        if version:
            logger.info(f"Extracted version from {release_pkg_name}: {version}")
            return version

    logger.info("CWD: %s", os.getcwd())
    logger.info("DIR CWD: %s", str(os.listdir(os.getcwd())))
    raise Exception(f"Failed to extract version from {release_pkg_name}")


def pip(machine: tap.Machine):
    return "pip3"


def python(machine: tap.Machine):
    return "python3"


def pip_install(machine: tap.Machine, *install_args: str):
    """Installs python packages onto a TAP machine"""
    pip_index = os.environ.get('TAP_PIP_INDEX_URL')
    pip_index_args = ['--index-url', pip_index] if pip_index else []
    pip_index_args += ["--no-cache-dir",
                       "--progress-bar", "off",
                       "--disable-pip-version-check",
                       "--default-timeout", "120"]
    machine.run(pip(machine), 'install', "pip", "--upgrade", *pip_index_args,
                log_mode=tap.LoggingMode.ON_ERROR)
    machine.run(pip(machine), '--log', '/opt/test/logs/pip.log',
                'install', *install_args, *pip_index_args,
                log_mode=tap.LoggingMode.ON_ERROR)


def package_install(machine: tap.Machine, *install_args: str):
    confirmation_arg = "-y"
    if machine.run('which', 'apt-get', return_exit_code=True) == 0:
        pkg_installer = "apt-get"
    elif machine.run('which', 'yum', return_exit_code=True) == 0:
        pkg_installer = "yum"
    else:
        pkg_installer = "zypper"
        confirmation_arg = "--non-interactive"

    for _ in range(20):
        if machine.run(pkg_installer, confirmation_arg, 'install', *install_args,
                       log_mode=tap.LoggingMode.ON_ERROR,
                       return_exit_code=True) == 0:
            break
        else:
            time.sleep(3)


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
        pip_install(machine, '-r', machine.inputs.test_scripts / 'requirements.txt')
        machine.run('groupadd', 'sophos-spl-group')
        machine.run('useradd', 'sophos-spl-user')
        machine.run('useradd', 'sophos-spl-local')
        package_install(machine, "rsync")
        package_install(machine, "openssl")
    except Exception as ex:
        # the previous command will fail if user already exists. But this is not an error
        print("On adding installing requirements: {}".format(ex))


def get_suffix():
    global BRANCH_NAME
    if BRANCH_NAME == "develop":
        return ""
    return "-" + BRANCH_NAME


def robot_task(machine: tap.Machine, robot_args: str):
    global BRANCH_NAME
    machine_name = machine.template
    try:
        install_requirements(machine)
        machine.run(robot_args, 'python3', machine.inputs.test_scripts / 'RobotFramework.py', timeout=3600)
    finally:
        machine.run('python3', machine.inputs.test_scripts / 'move_robot_results.py')
        machine.output_artifact('/opt/test/logs', 'logs')
        machine.output_artifact('/opt/test/results', 'results')
        machine.run('bash', UPLOAD_ROBOT_LOG_SCRIPT, "/opt/test/logs/log.html",
                    BRANCH_NAME + "/base" + get_suffix() + "_" + machine_name + "-log.html")


def coverage_task(machine: tap.Machine, branch: str, robot_args: str):
    try:
        install_requirements(machine)

        # upload unit test coverage-html and the unit-tests cov file to allegro
        unitest_htmldir = os.path.join(INPUTS_DIR, "sspl-base-unittest")
        machine.run('mv', str(machine.inputs.coverage_unittest), unitest_htmldir)
        machine.run('cp', COVFILE_UNITTEST, unitest_htmldir)
        machine.run('bash', '-x', UPLOAD_SCRIPT, environment={'UPLOAD_ONLY': 'UPLOAD', 'htmldir': unitest_htmldir, 'COVERAGE_TYPE': 'unit', 'COVFILE': COVFILE_UNITTEST})

        # publish unit test coverage file and results to artifactory results/coverage
        coverage_results_dir = os.path.join(RESULTS_DIR, 'coverage')
        machine.run('rm', '-rf', coverage_results_dir)
        machine.run('mkdir', coverage_results_dir)
        machine.run('cp', "-r", unitest_htmldir, coverage_results_dir)
        machine.run('cp', COVFILE_UNITTEST, coverage_results_dir)

        # run component pytests and integration robot tests with coverage file to get combined coverage
        machine.run('mv', COVFILE_UNITTEST, COVFILE_TAPTESTS)

        # "/tmp/BullseyeCoverageEnv.txt" is a special location that bullseye checks for config values
        # We can set the COVFILE env var here so that all instrumented processes know where it is.
        machine.run("echo", f"COVFILE={COVFILE_TAPTESTS}", ">", "/tmp/BullseyeCoverageEnv.txt")

        # Make sure that any product process can update the cov file, no matter the running user.
        machine.run("chmod", "666", COVFILE_TAPTESTS)

        # run component pytest -- these are disabled
        # run tap-tests
        try:
            machine.run(robot_args, 'python3', machine.inputs.test_scripts / 'RobotFramework.py',
                        timeout=3600,
                        environment={'COVFILE': COVFILE_TAPTESTS})
        finally:
            machine.run('python3', machine.inputs.test_scripts / 'move_robot_results.py')

        # generate tap (tap tests + unit tests) coverage html results and upload to allegro (and the .cov file which is in tap_htmldir)
        tap_htmldir = os.path.join(INPUTS_DIR, 'sspl-base-taptest')
        machine.run('mkdir', tap_htmldir)
        machine.run('cp', COVFILE_TAPTESTS, tap_htmldir)
        upload_results = "0"
        # Don't upload results to allegro unless we're on develop, can be overridden.
        if branch == SYSTEM_TEST_BULLSEYE_CI_BUILD_BRANCH:
            upload_results = "1"

        machine.run('bash', '-x', UPLOAD_SCRIPT,
                    environment={'COVFILE': COVFILE_TAPTESTS, 'BULLSEYE_UPLOAD': upload_results, 'htmldir': tap_htmldir})

        # publish tap (tap tests + unit tests) html results and coverage file to artifactory
        machine.run('mv', tap_htmldir, coverage_results_dir)
        machine.run('cp', COVFILE_TAPTESTS, coverage_results_dir)
        if branch == SYSTEM_TEST_BULLSEYE_CI_BUILD_BRANCH:
            #start systemtest coverage in jenkins (these include tap-tests)
            requests.get(url=SYSTEM_TEST_BULLSEYE_JENKINS_JOB_URL, verify=False)

    finally:
        machine.output_artifact('/opt/test/results', 'results')
        machine.output_artifact('/opt/test/logs', 'logs')


def unified_artifact(context: tap.PipelineContext, component: str, branch: str, sub_directory: str):
    """Return an input artifact from an unified pipeline build"""
    artifact = context.artifact.from_component(component, branch, org='', storage='esg-build-tested')
    # Using the truediv operator to set the sub directory forgets the storage option
    artifact.sub_directory = sub_directory
    return artifact


def get_inputs(context: tap.PipelineContext, base_build: ArtisanInput, mode: str):
    test_inputs = None
    if mode == 'release':
        test_inputs = dict(
            test_scripts=context.artifact.from_folder('./testUtils'),
            base_sdds=base_build / 'sspl-base/SDDS-COMPONENT',
            ra_sdds=base_build / 'sspl-base/RA-SDDS-COMPONENT',
            system_test=base_build / 'sspl-base/system_test',
            openssl=base_build / 'sspl-base' / 'openssl',
            websocket_server=context.artifact.from_component('liveterminal', 'prod', '1-0-267/219514') / 'websocket_server',
            bullseye_files=context.artifact.from_folder('./build/bullseye'),  # used for robot upload
        )
    if mode == 'debug':
        test_inputs = dict(
            test_scripts=context.artifact.from_folder('./testUtils'),
            base_sdds=base_build / 'sspl-base-debug/SDDS-COMPONENT',
            ra_sdds=base_build / 'sspl-base-debug/RA-SDDS-COMPONENT',
            system_test=base_build / 'sspl-base-debug/system_test',
            openssl=base_build / 'sspl-base-debug' / 'openssl',
            websocket_server=context.artifact.from_component('liveterminal', 'prod',
                                                             '1-0-267/219514') / 'websocket_server',
            bullseye_files=context.artifact.from_folder('./build/bullseye'),  # used for robot upload
        )
    if mode == 'coverage':
        test_inputs = dict(
            test_scripts=context.artifact.from_folder('./testUtils'),
            base_sdds=base_build / 'sspl-base-coverage/SDDS-COMPONENT',
            ra_sdds=base_build / 'sspl-base-coverage/RA-SDDS-COMPONENT',
            system_test=base_build / 'sspl-base-coverage/system_test',
            openssl=base_build / 'sspl-base-coverage' / 'openssl',
            websocket_server=context.artifact.from_component('liveterminal', 'prod',
                                                             '1-0-267/219514') / 'websocket_server',
            bullseye_files=context.artifact.from_folder('./build/bullseye'),
            coverage=base_build / 'sspl-base-coverage/covfile',
            coverage_unittest=base_build / 'sspl-base-coverage/unittest-htmlreport',
            bazel_tools=unified_artifact(context, 'em.esg', 'develop', 'build/bazel-tools')
        )
    return test_inputs


def get_test_machines(test_inputs, parameters):
    if parameters.run_tests == "false":
        return []

    test_environments = {}

    if parameters.run_amazon_2 != "false":
        test_environments['amazonlinux2'] = 'amzlinux2_x64_server_en_us'

    if parameters.run_amazon_2023 != "false":
        test_environments['amazonlinux2023'] = 'amzlinux2023_x64_server_en_us'

    if parameters.run_centos_7 != "false":
        test_environments['centos79'] = 'centos7_x64_aws_server_en_us'

    if parameters.run_centos_stream_8 != "false":
        test_environments['centos8stream'] = 'centos8stream_x64_aws_server_en_us'

    if parameters.run_centos_stream_9 != "false":
        test_environments['centos9stream'] = 'centos9stream_x64_aws_server_en_us'

    if parameters.run_debian_10 != "false":
        test_environments['debian10'] = 'debian10_x64_aws_server_en_us'

    if parameters.run_debian_11 != "false":
        test_environments['debian11'] = 'debian11_x64_aws_server_en_us'

    if parameters.run_oracle_7 != "false":
        test_environments['oracle7'] = 'oracle79_x64_aws_server_en_us'

    if parameters.run_oracle_8 != "false":
        test_environments['oracle8'] = 'oracle87_x64_aws_server_en_us'

    if parameters.run_rhel_7 != "false":
        test_environments['rhel7'] = 'rhel79_x64_aws_server_en_us'

    if parameters.run_rhel_8 != "false":
        test_environments['rhel8'] = 'rhel87_x64_aws_server_en_us'

    if parameters.run_rhel_9 != "false":
        test_environments['rhel9'] = 'rhel91_x64_aws_server_en_us'

    if parameters.run_sles_12 != "false":
        test_environments['sles12'] = 'sles12_x64_sp5_aws_server_en_us'

    if parameters.run_sles_15 != "false":
        test_environments['sles15'] = 'sles15_x64_sp4_aws_server_en_us'

    if parameters.run_ubuntu_18_04 != "false":
        test_environments['ubuntu1804'] = 'ubuntu1804_x64_aws_server_en_us'

    if parameters.run_ubuntu_20_04 != "false":
        test_environments['ubuntu2004'] = 'ubuntu2004_x64_aws_server_en_us'

    # TODO: LINUXDAR-7306 set CIJenkins to default=true once python3.10 issues are resolved
    if parameters.run_ubuntu_22_04 != "false":
        test_environments['ubuntu2204'] = 'ubuntu2204_x64_aws_server_en_us'

    ret = []
    for name, image in test_environments.items():
        ret.append((
            name,
            tap.Machine(image, inputs=test_inputs, platform=tap.Platform.Linux)
        ))
    return ret


def build_release(stage: tap.Root, component: tap.Component):
    return stage.artisan_build(
        name=RELEASE_MODE, component=component, image=BUILD_TEMPLATE,
        mode=RELEASE_MODE, release_package=RELEASE_PKG)

def build_debug(stage: tap.Root, component: tap.Component):
    return stage.artisan_build(
        name=DEBUG_MODE, component=component, image=BUILD_TEMPLATE,
        mode=DEBUG_MODE, release_package=RELEASE_PKG)

def build_analysis(stage: tap.Root, component: tap.Component):
    return stage.artisan_build(
        name=ANALYSIS_MODE, component=component, image=BUILD_TEMPLATE,
        mode=ANALYSIS_MODE, release_package=RELEASE_PKG)

def build_coverage(stage: tap.Root, component: tap.Component):
    return stage.artisan_build(
        name=COVERAGE_MODE, component=component, image=BUILD_TEMPLATE,
        mode=COVERAGE_MODE, release_package=RELEASE_PKG)

def build_999(stage: tap.Root, component: tap.Component):
    return stage.artisan_build(
        name=NINE_NINE_NINE_MODE, component=component, image=BUILD_TEMPLATE,
        mode=NINE_NINE_NINE_MODE, release_package=RELEASE_PKG)

def build_060(stage: tap.Root, component: tap.Component):
    return stage.artisan_build(
        name=ZERO_SIX_ZERO_MODE, component=component, image=BUILD_TEMPLATE,
        mode=ZERO_SIX_ZERO_MODE, release_package=RELEASE_PKG)

def build_bazel(stage: tap.Root, component: tap.Component):
    return stage.artisan_build(
        name="bazel", component=component, image=BUILD_TEMPLATE_BAZEL,
        mode="bazel_release", release_package=BAZEL_RELEASE_PKG)


@tap.pipeline(version=1, component=COMPONENT, root_sequential=False)
def sspl_base(stage: tap.Root, context: tap.PipelineContext, parameters: tap.Parameters):
    global BRANCH_NAME
    BRANCH_NAME = context.branch

    component = tap.Component(name=COMPONENT, base_version=get_base_version())
    # For cmdline/local builds, determine build mode by how tap was called
    # Can override this with `TAP_PARAMETER_MODE=debug` prefacing tap command
    determined_build_mode = None
    if f"{component.name}.build.{DEBUG_MODE}" in sys.argv:
        determined_build_mode = DEBUG_MODE
    if f"{component.name}.build.{COVERAGE_MODE}" in sys.argv:
        determined_build_mode = COVERAGE_MODE
    # ls logic check here acts to make all builds default to release apart from "tap ls" if
    # another build mode cannot be determined. Tap ls needs to have no build mode to see all builds
    elif f"{component.name}.build.{RELEASE_MODE}" in sys.argv or sys.argv[1] != 'ls':
        determined_build_mode = RELEASE_MODE

    # In CI parameters.mode will be set
    mode = parameters.mode or determined_build_mode

    base_build = None
    INCLUDE_BUILD_IN_PIPELINE = parameters.get('INCLUDE_BUILD_IN_PIPELINE', True)
    if INCLUDE_BUILD_IN_PIPELINE:
        with stage.parallel('build'):
            if mode:
                if mode == RELEASE_MODE or mode == ANALYSIS_MODE:
                    build_analysis(stage, component)
                    base_build = build_release(stage, component)
                    build_999(stage, component)
                    build_060(stage, component)
                elif mode == DEBUG_MODE:
                    base_build = build_debug(stage, component)
                    build_999(stage, component)
                    build_060(stage, component)
                elif mode == COVERAGE_MODE:
                    base_build = build_coverage(stage, component)
                    build_release(stage, component)
                    build_999(stage, component)
                    build_060(stage, component)
            else:
                # For "tap ls" to work the default path through here with no params etc. must be to run all builds,
                # else only the default build path, which used to be release will be listed.
                base_build = build_release(stage, component)
                build_debug(stage, component)
                build_analysis(stage, component)
                build_coverage(stage, component)
                build_999(stage, component)
                build_060(stage, component)
            if parameters.bazel_build != "False":
                build_bazel(stage, component)
    else:
        base_build = context.artifact.build()

    # Modes where we do not want to run TAP tests.
    if mode in [ANALYSIS_MODE]:
        return

    test_inputs = get_inputs(context, base_build, mode)
    machines = get_test_machines(test_inputs, parameters)

    # Add args to pass env vars to RobotFramework.py call in test runs
    robot_args_list = []
    if mode == DEBUG_MODE:
        robot_args_list.append("DEBUG=true")
    if parameters.robot_test:
        robot_args_list.append("TEST=" + parameters.robot_test)
    if parameters.robot_suite:
        robot_args_list.append("SUITE=" + parameters.robot_suite)
    robot_args = " ".join(robot_args_list)

    with stage.parallel('integration'):
        if mode == COVERAGE_MODE:
            stage.task(task_name="centos7", func=coverage_task,
                       machine=tap.Machine('centos7_x64_aws_server_en_us', inputs=test_inputs,
                                           platform=tap.Platform.Linux), branch=context.branch,
                       robot_args=robot_args)
        else:
            for template_name, machine in machines:
                stage.task(task_name=template_name, func=robot_task, machine=machine, robot_args=robot_args)

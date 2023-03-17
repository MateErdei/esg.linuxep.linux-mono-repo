import sys
import tap.v1 as tap
import xml.etree.ElementTree as ET
from tap._pipeline.tasks import ArtisanInput
import os
import requests

SYSTEM_TEST_BULLSEYE_JENKINS_JOB_URL = 'https://sspljenkins.eng.sophos/job/SSPL-Plugin-Event-Journaler-bullseye-system-test-coverage/build?token=sspl-linuxdarwin-coverage-token'
# For branch names, remember that slashes are replaced with hyphens:  '/' -> '-'
SYSTEM_TEST_BULLSEYE_CI_BUILD_BRANCH = 'develop'

COVFILE_UNITTEST = '/opt/test/inputs/coverage/sspl-plugin-eventjournaler-unit.cov'
COVFILE_TAPTESTS = '/opt/test/inputs/coverage/sspl-plugin-eventjournaler-tap.cov'
UPLOAD_SCRIPT = '/opt/test/inputs/bullseye_files/uploadResults.sh'
LOGS_DIR = '/opt/test/logs'
RESULTS_DIR = '/opt/test/results'
INPUTS_DIR = '/opt/test/inputs'

RELEASE_MODE = 'release'
ANALYSIS_MODE = 'analysis'
COVERAGE_MODE = 'coverage'
NINE_NINE_NINE_MODE = '999'
ZERO_SIX_ZERO_MODE = '060'

def coverage_task(machine: tap.Machine, branch: str, robot_args=None):
    try:
        install_requirements(machine)
        tests_dir = str(machine.inputs.test_scripts)

        # upload unit test coverage html results to allegro (and the .cov file which is in unitest_htmldir)
        unitest_htmldir = os.path.join(INPUTS_DIR, "sspl-plugin-eventjournaler-unittest")
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

        # run component pytest and tap-tests
        try:
            machine.run(robot_args, 'python3', machine.inputs.test_scripts / 'RobotFramework.py', timeout=3600,
                        environment={'COVFILE': COVFILE_TAPTESTS})
        finally:
            machine.run('python3', machine.inputs.test_scripts / 'move_robot_results.py')

        # generate tap (tap tests + unit tests) coverage html results and upload to allegro (and the .cov file which is in tap_htmldir)
        tap_htmldir = os.path.join(INPUTS_DIR, 'sspl-plugin-eventjournaler-taptest')
        machine.run ('mkdir', tap_htmldir)
        machine.run('cp', COVFILE_TAPTESTS, tap_htmldir)
        machine.run('bash', '-x', UPLOAD_SCRIPT,
                    environment={'COVFILE': COVFILE_TAPTESTS, 'BULLSEYE_UPLOAD': '1', 'htmldir': tap_htmldir})

        # publish tap (tap tests + unit tests) html results and coverage file to artifactory
        machine.run('mv', tap_htmldir, coverage_results_dir)
        machine.run('cp', COVFILE_TAPTESTS, coverage_results_dir)

        #trigger system test coverage job on jenkins - this will also upload to allegro
        if branch == SYSTEM_TEST_BULLSEYE_CI_BUILD_BRANCH:
            requests.get(url=SYSTEM_TEST_BULLSEYE_JENKINS_JOB_URL, verify=False)

    finally:
        machine.output_artifact('/opt/test/results', 'results')
        machine.output_artifact('/opt/test/logs', 'logs')


def get_package_version(RELEASE_PKG):
    """ Read version from package.xml """
    package_tree = ET.parse(RELEASE_PKG)
    package_node = package_tree.getroot()
    return package_node.attrib['version']

BUILD_TEMPLATE = 'centos79_x64_build_20230202'
RELEASE_PKG = './build-files/release-package.xml'
PACKAGE_VERSION = get_package_version(RELEASE_PKG)
def install_requirements(machine: tap.Machine):
    """ install python lib requirements """
    pip_install(machine, '-r', machine.inputs.test_scripts / 'requirements.txt')


def pip_install(machine: tap.Machine, *install_args: str):
    """Installs python packages onto a TAP machine"""
    pip_index = os.environ.get('TAP_PIP_INDEX_URL')
    pip_index_args = ['--index-url', pip_index] if pip_index else []
    pip_index_args += ["--no-cache-dir",
                       "--progress-bar", "off",
                       "--disable-pip-version-check",
                       "--default-timeout", "120"]
    machine.run('pip3', '--log', '/opt/test/logs/pip.log',
                'install', *install_args, *pip_index_args,
                log_mode=tap.LoggingMode.ON_ERROR)


def robot_task(machine: tap.Machine, robot_args: str):
    try:
        install_requirements(machine)
        machine.run(robot_args, 'python3', machine.inputs.test_scripts / 'RobotFramework.py', timeout=3600)
    finally:
        machine.run('python3', machine.inputs.test_scripts / 'move_robot_results.py')
        machine.output_artifact('/opt/test/logs', 'logs')
        machine.output_artifact('/opt/test/results', 'results')

def unified_artifact(context: tap.PipelineContext, component: str, branch: str, sub_directory: str):
    """Return an input artifact from an unified pipeline build"""
    artifact = context.artifact.from_component(component, branch, org='', storage='esg-build-tested')
    # Using the truediv operator to set the sub directory forgets the storage option
    artifact.sub_directory = sub_directory
    return artifact

def get_inputs(context: tap.PipelineContext, ej_build: ArtisanInput, mode: str):
    test_inputs = None
    if mode == 'release':
        test_inputs = dict(
            test_scripts=context.artifact.from_folder('./TA'),
            event_journaler_sdds=ej_build / 'eventjournaler/SDDS-COMPONENT',
            manual_tools=ej_build / 'eventjournaler/manualTools',
            base_sdds=ej_build / 'base/base-sdds',
            fake_management=ej_build / 'base/fake-management'
        )
    if mode == 'coverage':
        test_inputs = dict(
            test_scripts=context.artifact.from_folder('./TA'),
            event_journaler_sdds=ej_build / 'sspl-plugin-eventjournaler-coverage/SDDS-COMPONENT',
            manual_tools=ej_build / 'sspl-plugin-eventjournaler-coverage/manualTools',
            bullseye_files=context.artifact.from_folder('./build/bullseye'),
            coverage=ej_build / 'sspl-plugin-eventjournaler-coverage/covfile',
            coverage_unittest=ej_build / 'sspl-plugin-eventjournaler-coverage/unittest-htmlreport',
            base_sdds=ej_build / 'sspl-plugin-eventjournaler-coverage/base/base-sdds',
            fake_management=ej_build / 'sspl-plugin-eventjournaler-coverage/base/fake-management',
            bazel_tools=unified_artifact(context, 'em.esg', 'develop', 'build/bazel-tools')
        )
    return test_inputs


def build_release(stage: tap.Root, component: tap.Component):
    return stage.artisan_build(
        name=RELEASE_MODE, component=component, image=BUILD_TEMPLATE,
        mode=RELEASE_MODE, release_package=RELEASE_PKG)


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

@tap.pipeline(version=1, component='sspl-event-journaler-plugin')
def event_journaler(stage: tap.Root, context: tap.PipelineContext, parameters: tap.Parameters):
    component = tap.Component(name='sspl-event-journaler-plugin', base_version=PACKAGE_VERSION)

    # For cmdline/local builds, determine build mode by how tap was called
    # Can override this with `TAP_PARAMETER_MODE=debug` prefacing tap command
    component_argument_name = "event_journaler"
    determined_build_mode = None
    if f"{component_argument_name}.build.{ANALYSIS_MODE}" in sys.argv:
        determined_build_mode = ANALYSIS_MODE
    if f"{component_argument_name}.build.{COVERAGE_MODE}" in sys.argv:
        determined_build_mode = COVERAGE_MODE
    # ls logic check here acts to make all builds default to release apart from "tap ls" if
    # another build mode cannot be determined. Tap ls needs to have no build mode to see all builds
    elif f"{component_argument_name}.build.{RELEASE_MODE}" in sys.argv or sys.argv[1] != 'ls':
        determined_build_mode = RELEASE_MODE

    # In CI parameters.mode will be set
    mode = parameters.mode or determined_build_mode

    ej_build = None
    with stage.parallel('build'):
        if mode:
            if mode == RELEASE_MODE or mode == ANALYSIS_MODE:
                build_analysis(stage, component)
                ej_build = build_release(stage, component)
                build_999(stage, component)
                build_060(stage, component)
            elif mode == COVERAGE_MODE:
                ej_build = build_coverage(stage, component)
                build_release(stage, component)
                build_999(stage, component)
                build_060(stage, component)
        else:
            # For "tap ls" to work the default path through here with no params etc. must be to run all builds,
            # else only the default build path, which used to be release will be listed.
            ej_build = build_release(stage, component)
            build_analysis(stage, component)
            build_coverage(stage, component)
            build_999(stage, component)
            build_060(stage, component)

    # Modes where we do not want to run TAP tests.
    if mode in [ANALYSIS_MODE]:
        return

    test_inputs = get_inputs(context, ej_build, mode)
    machines = (
        ("ubuntu1804",
         tap.Machine('ubuntu1804_x64_server_en_us', inputs=test_inputs, platform=tap.Platform.Linux)),

        ("ubuntu2004",
         tap.Machine('ubuntu2004_x64_server_en_us', inputs=test_inputs, platform=tap.Platform.Linux)),

        ("ubuntu2204",
         tap.Machine('ubuntu2204_x64_aws_server_en_us', inputs=test_inputs, platform=tap.Platform.Linux)),

        ("centos79",
         tap.Machine('centos79_x64_server_en_us', inputs=test_inputs, platform=tap.Platform.Linux)),

        ("centos84",
         tap.Machine('centos84_x64_server_en_us', inputs=test_inputs, platform=tap.Platform.Linux)),

        ("centos8stream",
         tap.Machine('centos8stream_x64_aws_server_en_us', inputs=test_inputs, platform=tap.Platform.Linux)),

        ("centos9stream",
         tap.Machine('centos9stream_x64_aws_server_en_us', inputs=test_inputs, platform=tap.Platform.Linux)),

        ("amazonlinux2",
         tap.Machine('amzlinux2_x64_server_en_us', inputs=test_inputs, platform=tap.Platform.Linux)),

        ("sles15",
         tap.Machine('sles15_x64_sp4_aws_server_en_us', inputs=test_inputs, platform=tap.Platform.Linux)),
        # add other distros here
    )

    # Add args to pass env vars to RobotFramework.py call in test runs
    robot_args_list = []
    if parameters.test:
        robot_args_list.append("TEST=" + parameters.test)
    if parameters.suite:
        robot_args_list.append("SUITE=" + parameters.suite)
    robot_args = " ".join(robot_args_list)

    with stage.parallel('integration'):
        if mode == COVERAGE_MODE:
            stage.task(task_name="centos77", func=coverage_task,
                       machine=tap.Machine('centos77_x64_server_en_us', inputs=test_inputs,
                                           platform=tap.Platform.Linux), branch=context.branch,
                       robot_args=robot_args)
        else:
            for template_name, machine in machines:
                stage.task(task_name=template_name, func=robot_task, machine=machine, robot_args=robot_args)

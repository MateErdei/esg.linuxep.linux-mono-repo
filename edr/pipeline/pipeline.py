import sys
import tap.v1 as tap
from tap._pipeline.tasks import ArtisanInput
import os
import logging
import requests
import xml.etree.ElementTree as ET

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

RELEASE_MODE = 'release'
ANALYSIS_MODE = 'analysis'
COVERAGE_MODE = 'coverage'
NINE_NINE_NINE_MODE = '999'

BUILD_TEMPLATE = 'centos79_x64_build_20230202'
RELEASE_PACKAGE = './build-files/release-package.xml'

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


def has_coverage_build(branch_name):
    """If the branch name does an analysis mode build"""
    return branch_name == 'develop' or branch_name.endswith('coverage')


def has_coverage_file(machine: tap.Machine):
    """If the downloaded build output has a coverage file then its a bullseye build"""
    return machine.run('test', '-f', COVFILE_UNITTEST, return_exit_code=True) == 0


def install_requirements(machine: tap.Machine):
    """ install python lib requirements """
    pip_install(machine, '-r', machine.inputs.test_scripts / 'requirements.txt')
    try:
        machine.run('useradd', 'sophos-spl-user')
        machine.run('useradd', 'sophos-spl-local')
        machine.run('groupadd', 'sophos-spl-group')
    except Exception as ex:
        # the previous command will fail if user already exists. But this is not an error
        logger.warning("On adding user and group: {}".format(ex))


def robot_task(machine: tap.Machine, robot_args: str):
    try:
        install_requirements(machine)
        machine.run(robot_args, 'python3', machine.inputs.test_scripts / 'RobotFramework.py', timeout=3600)
    finally:
        machine.run('python3', machine.inputs.test_scripts / 'move_robot_results.py')
        machine.output_artifact('/opt/test/logs', 'logs')
        machine.output_artifact('/opt/test/results', 'results')


def coverage_task(machine: tap.Machine, branch: str, robot_args: str):
    try:
        install_requirements(machine)
        tests_dir = str(machine.inputs.test_scripts)

        # upload unit test coverage html results to allegro (and the .cov file which is in unitest_htmldir)
        unitest_htmldir = os.path.join(INPUTS_DIR, "sspl-plugin-edr-unittest")
        machine.run('mv', str(machine.inputs.coverage_unittest), unitest_htmldir)
        machine.run('bash', '-x', UPLOAD_SCRIPT, environment={'UPLOAD_ONLY': 'UPLOAD', 'htmldir': unitest_htmldir, 'COVFILE': COVFILE_UNITTEST, 'COVERAGE_TYPE': 'unit'})

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
        args = ['python3', '-u', '-m', 'pytest', tests_dir, '--html=/opt/test/results/report.html']
        machine.run(*args, environment={'COVFILE': COVFILE_TAPTESTS})
        try:
            machine.run(robot_args,'python3', machine.inputs.test_scripts / 'RobotFramework.py', timeout=3600,
                        environment={'COVFILE': COVFILE_TAPTESTS})
        finally:
            machine.run('python3', machine.inputs.test_scripts / 'move_robot_results.py')

        # generate tap (tap tests + unit tests) coverage html results and upload to allegro (and the .cov file which is in tap_htmldir)
        tap_htmldir = os.path.join(INPUTS_DIR, 'sspl-plugin-edr-taptest')
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


def pytest_task(machine: tap.Machine):
    try:
        install_requirements(machine)
        tests_dir = str(machine.inputs.test_scripts)

        # To run the pytest with additional verbosity add following to arguments
        # '-o', 'log_cli=true'
        args = ['python3', '-u', '-m', 'pytest', tests_dir, '--html=/opt/test/results/report.html']
        machine.run(*args)
        machine.run('ls', '/opt/test/logs')
    finally:
        machine.output_artifact('/opt/test/results', 'results')
        machine.output_artifact('/opt/test/logs', 'logs')

def unified_artifact(context: tap.PipelineContext, component: str, branch: str, sub_directory: str):
    """Return an input artifact from an unified pipeline build"""
    artifact = context.artifact.from_component(component, branch, org='', storage='esg-build-tested')
    # Using the truediv operator to set the sub directory forgets the storage option
    artifact.sub_directory = sub_directory
    return artifact


def get_inputs(context: tap.PipelineContext, edr_build: ArtisanInput, mode: str):
    test_inputs = None
    if mode == 'release':
        test_inputs = dict(
            test_scripts=context.artifact.from_folder('./TA'),
            edr_sdds=edr_build / 'edr/SDDS-COMPONENT',
            base_sdds=edr_build / 'base/base-sdds',
            componenttests=edr_build / 'componenttests',
            qp=unified_artifact(context, 'em.esg', 'develop', 'build/scheduled-query-pack-sdds'),
            lp=unified_artifact(context, 'em.esg', 'develop', 'build/endpoint-query-pack')
        )
    if mode == 'coverage':
        test_inputs = dict(
            test_scripts=context.artifact.from_folder('./TA'),
            edr_sdds=edr_build / 'sspl-edr-coverage/SDDS-COMPONENT',
            bullseye_files=context.artifact.from_folder('./build/bullseye'),
            coverage=edr_build / 'sspl-edr-coverage/covfile',
            coverage_unittest=edr_build / 'sspl-edr-coverage/unittest-htmlreport',
            base_sdds=edr_build / 'sspl-edr-coverage/base/base-sdds',
            componenttests=edr_build / 'sspl-edr-coverage/componenttests',
            qp=unified_artifact(context, 'em.esg', 'develop', 'build/scheduled-query-pack-sdds'),
            lp=unified_artifact(context, 'em.esg', 'develop', 'build/endpoint-query-pack'),
            bazel_tools=unified_artifact(context, 'em.esg', 'develop', 'build/bazel-tools')
        )

    return test_inputs

def get_package_version(package_path):
    """ Read version from package.xml """
    package_tree = ET.parse(package_path)
    package_node = package_tree.getroot()
    return package_node.attrib['version']


def build_release(stage: tap.Root, component: tap.Component):
    return stage.artisan_build(
        name=RELEASE_MODE, component=component, image=BUILD_TEMPLATE,
        mode=RELEASE_MODE, release_package=RELEASE_PACKAGE)

def build_analysis(stage: tap.Root, component: tap.Component):
    return stage.artisan_build(
        name=ANALYSIS_MODE, component=component, image=BUILD_TEMPLATE,
        mode=ANALYSIS_MODE, release_package=RELEASE_PACKAGE)

def build_coverage(stage: tap.Root, component: tap.Component):
    return stage.artisan_build(
        name=COVERAGE_MODE, component=component, image=BUILD_TEMPLATE,
        mode=COVERAGE_MODE, release_package=RELEASE_PACKAGE)

def build_999(stage: tap.Root, component: tap.Component):
    return stage.artisan_build(
        name=NINE_NINE_NINE_MODE, component=component, image=BUILD_TEMPLATE,
        mode=NINE_NINE_NINE_MODE, release_package=RELEASE_PACKAGE)


def get_test_machines(test_inputs, parameters: tap.Parameters):
    test_environments = {'ubuntu1804': 'ubuntu1804_x64_server_en_us',
                         'ubuntu2004': 'ubuntu2004_x64_server_en_us',
                         #TODO: Fix broken yum repo to point at abn-engrepo.eng.sophos instead of abn-centosrepo
                         #'centos79': 'centos79_x64_server_en_us',
                         'centos84': 'centos84_x64_server_en_us',
                         'centos8stream': 'centos8stream_x64_aws_server_en_us',
                         #TODO: Fix SELinux rules to allow access to /opt/sophos-spl/shared/syslog_pipe
                         #'centos9stream': 'centos9stream_x64_aws_server_en_us',
                         'amazonlinux2': 'amzlinux2_x64_server_en_us',
                         'oracle8': 'oracle87_x64_aws_server_en_us',
                         }
    if parameters.run_sles != 'false':
        test_environments['sles12'] = 'sles12_x64_sp5_aws_server_en_us'
        test_environments['sles15'] = 'sles15_x64_sp4_aws_server_en_us'

    if parameters.run_ubuntu_22_04 != 'false':
        test_environments['ubuntu2204'] = 'ubuntu2204_x64_aws_server_en_us'

    ret = []
    for name, image in test_environments.items():
        ret.append((
            name,
            tap.Machine(image, inputs=test_inputs, platform=tap.Platform.Linux)
        ))
    return ret

@tap.pipeline(version=1, component='sspl-plugin-edr-component')
def edr_plugin(stage: tap.Root, context: tap.PipelineContext, parameters: tap.Parameters):

    # For cmdline/local builds, determine build mode by how tap was called
    # Can override this with `TAP_PARAMETER_MODE=debug` prefacing tap command
    component_argument_name = "edr_plugin"
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
    component = tap.Component(name='edr', base_version=get_package_version(RELEASE_PACKAGE))

    #export TAP_PARAMETER_MODE=release|analysis|coverage*(requires bullseye)
    edr_build = None
    with stage.parallel('build'):
        if mode == RELEASE_MODE or mode == ANALYSIS_MODE:
            build_analysis(stage, component)
            edr_build = build_release(stage, component)
            build_999(stage, component)
        elif mode == COVERAGE_MODE:
            edr_build = build_coverage(stage, component)
            build_release(stage, component)
            build_999(stage, component)
        else:
            build_release(stage, component)
            build_analysis(stage, component)
            build_coverage(stage, component)
            build_999(stage, component)

    if mode == ANALYSIS_MODE:
        return

    # Add args to pass env vars to RobotFramework.py call in test runs
    robot_args_list = []
    if parameters.test:
        robot_args_list.append("TEST=" + parameters.test)
    if parameters.suite:
        robot_args_list.append("SUITE=" + parameters.suite)
    robot_args = " ".join(robot_args_list)

    with stage.parallel('test'):
        test_inputs = get_inputs(context, edr_build, mode)

        if mode == 'coverage':
            with stage.parallel('combined'):
                coverage_machines = (
                    ("centos77", tap.Machine('centos77_x64_server_en_us', inputs=get_inputs(context, edr_build, mode), platform=tap.Platform.Linux)),
                )
                for template_name, machine in coverage_machines:
                    stage.task(task_name=template_name, func=coverage_task, machine=machine, branch=context.branch, robot_args=robot_args)
        else:
            with stage.parallel('integration'):
                for template_name, machine in get_test_machines(test_inputs, parameters):
                    stage.task(task_name=template_name, func=robot_task, machine=machine, robot_args=robot_args)

            with stage.parallel('component'):
                for template_name, machine in get_test_machines(test_inputs, parameters):
                    stage.task(task_name=template_name, func=pytest_task, machine=machine)

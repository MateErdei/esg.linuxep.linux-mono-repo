import tap.v1 as tap
import xml.etree.ElementTree as ET
from tap._pipeline.tasks import ArtisanInput
import os
import requests

SYSTEM_TEST_BULLSEYE_JENKINS_JOB_URL = 'https://sspljenkins.eng.sophos/job/SSPL-Plugin-Event-Journaler-bullseye-system-test-coverage/build?token=sspl-linuxdarwin-coverage-token'
COVFILE_UNITTEST = '/opt/test/inputs/coverage/sspl-plugin-eventjournaler-unit.cov'
COVFILE_TAPTESTS = '/opt/test/inputs/coverage/sspl-plugin-eventjournaler-tap.cov'
UPLOAD_SCRIPT = '/opt/test/inputs/bullseye_files/uploadResults.sh'
LOGS_DIR = '/opt/test/logs'
RESULTS_DIR = '/opt/test/results'
INPUTS_DIR = '/opt/test/inputs'

def combined_task(machine: tap.Machine, branch: str):
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
            machine.run('python3', machine.inputs.test_scripts / 'RobotFramework.py', timeout=3600,
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

        #trigger system test coverage job on jenkins on develop branch - this will also upload to allegro
        if branch == 'develop':
            requests.get(url=SYSTEM_TEST_BULLSEYE_JENKINS_JOB_URL, verify=False)

    finally:
        machine.output_artifact('/opt/test/results', 'results')
        machine.output_artifact('/opt/test/logs', 'logs')


def get_package_version(package_path):
    """ Read version from package.xml """
    package_tree = ET.parse(package_path)
    package_node = package_tree.getroot()
    return package_node.attrib['version']

PACKAGE_PATH = './build-files/release-package.xml'
PACKAGE_VERSION = get_package_version(PACKAGE_PATH)
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


def robot_task(machine: tap.Machine):
    try:
        install_requirements(machine)
        machine.run('python3', machine.inputs.test_scripts / 'RobotFramework.py', timeout=3600)
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
            base_sdds=ej_build / 'base/base-sdds'
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
            bazel_tools=unified_artifact(context, 'em.esg', 'develop', 'build/bazel-tools')
        )
    return test_inputs

@tap.pipeline(version=1, component='sspl-event-journaler-plugin')
def event_journaler(stage: tap.Root, context: tap.PipelineContext, parameters: tap.Parameters):
    component = tap.Component(name='sspl-event-journaler-plugin', base_version=PACKAGE_VERSION)

    release_mode = 'release'
    analysis_mode = 'analysis'
    coverage_mode = 'coverage'
    nine_nine_nine_mode = '999'
    zero_six_zero_mode = '060'

    mode = parameters.mode or release_mode

    ej_build = None
    with stage.parallel('build'):
        if mode == release_mode or mode == analysis_mode:
            ej_build = stage.artisan_build(name=release_mode, component=component, image='JenkinsLinuxTemplate6',
                                            mode=release_mode, release_package=PACKAGE_PATH)
            nine_nine_nine_build = stage.artisan_build(name=nine_nine_nine_mode, component=component, image='JenkinsLinuxTemplate6',
                                                       mode=nine_nine_nine_mode, release_package=PACKAGE_PATH)
            zero_six_zero = stage.artisan_build(name=zero_six_zero_mode, component=component, image='JenkinsLinuxTemplate6',
                                                mode=zero_six_zero_mode, release_package=PACKAGE_PATH)
            analysis_build = stage.artisan_build(name=analysis_mode, component=component, image='JenkinsLinuxTemplate6',
                                                 mode=analysis_mode, release_package=PACKAGE_PATH)
        elif mode == coverage_mode:
            release_build = stage.artisan_build(name=release_mode, component=component, image='JenkinsLinuxTemplate6',
                                                mode=release_mode, release_package=PACKAGE_PATH)
            ej_build = stage.artisan_build(name=coverage_mode, component=component, image='JenkinsLinuxTemplate6',
                                                 mode=coverage_mode, release_package=PACKAGE_PATH)
            nine_nine_nine_build = stage.artisan_build(name=nine_nine_nine_mode, component=component, image='JenkinsLinuxTemplate6',
                                                       mode=nine_nine_nine_mode, release_package=PACKAGE_PATH)
            zero_six_zero = stage.artisan_build(name=zero_six_zero_mode, component=component, image='JenkinsLinuxTemplate6',
                                                mode=zero_six_zero_mode, release_package=PACKAGE_PATH)

    if mode == analysis_mode:
        return

    with stage.parallel('test'):
        machines = (
            ("ubuntu1804",
             tap.Machine('ubuntu1804_x64_server_en_us', inputs=get_inputs(context, ej_build, mode), platform=tap.Platform.Linux)),
            ("centos77", tap.Machine('centos77_x64_server_en_us', inputs=get_inputs(context, ej_build, mode), platform=tap.Platform.Linux)),
            ("centos82", tap.Machine('centos82_x64_server_en_us', inputs=get_inputs(context, ej_build, mode), platform=tap.Platform.Linux)),
            # add other distros here
        )
        coverage_machines = (
            ("centos77", tap.Machine('centos77_x64_server_en_us', inputs=get_inputs(context, ej_build, mode), platform=tap.Platform.Linux)),
        )

    if mode == 'coverage':
        with stage.parallel('combined'):
            for template_name, machine in coverage_machines:
                stage.task(task_name=template_name, func=combined_task, machine=machine, branch=context.branch)
    else:
        with stage.parallel('integration'):
            for template_name, machine in machines:
                stage.task(task_name=template_name, func=robot_task, machine=machine)

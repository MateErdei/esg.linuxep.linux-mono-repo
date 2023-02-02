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

COVFILE_UNITTEST = '/opt/test/inputs/coverage/sspl-base-unittest.cov'
COVFILE_TAPTESTS = '/opt/test/inputs/coverage/sspl-base-taptests.cov'
UPLOAD_SCRIPT = '/opt/test/inputs/bullseye_files/uploadResults.sh'

RESULTS_DIR = '/opt/test/results'
INPUTS_DIR = '/opt/test/inputs'

COMPONENT = 'sspl_base'
BUILD_TEMPLATE = 'JenkinsLinuxTemplate9'
RELEASE_PKG = './build/release-package.xml'
RELEASE_MODE = 'release'
DEBUG_MODE = 'debug'
ANALYSIS_MODE = 'analysis'
COVERAGE_MODE = 'coverage'
NINE_NINE_NINE_MODE = '999'
ZERO_SIX_ZERO_MODE = '060'

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

def package_install(machine: tap.Machine, *install_args: str):
    if machine.run('which', 'apt-get', return_exit_code=True) == 0:
        pkg_installer = "apt-get"
    else:
        pkg_installer = "yum"

    for _ in range(20):
        if machine.run(pkg_installer, '-y', 'install', *install_args,
                       log_mode=tap.LoggingMode.ON_ERROR,
                       return_exit_code=True) == 0:
            break
        else:
            time.sleep(3)

def install_requirements(machine: tap.Machine):
    """ install python lib requirements """
    try:
        machine.run('pip3', 'install', "pip", "--upgrade")
        pip_install(machine, '-r', machine.inputs.test_scripts / 'requirements.txt')
        machine.run('useradd', 'sophos-spl-user')
        machine.run('useradd', 'sophos-spl-local')
        machine.run('groupadd', 'sophos-spl-group')
    except Exception as ex:
        # the previous command will fail if user already exists. But this is not an error
        print("On adding installing requirements: {}".format(ex))

def robot_task(machine: tap.Machine):
    try:
        if machine.run('which', 'apt-get', return_exit_code=True) == 0:
            package_install(machine, 'python3.7-dev')
        install_requirements(machine)
        if DEBUG_MODE:
            machine.run('DEBUG=true', 'python3', machine.inputs.test_scripts / 'RobotFramework.py',
                        timeout=3600)
        else:
            machine.run('python3', machine.inputs.test_scripts / 'RobotFramework.py',
                    timeout=3600)
    finally:
        machine.run('python3', machine.inputs.test_scripts / 'move_robot_results.py')
        machine.output_artifact('/opt/test/logs', 'logs')
        machine.output_artifact('/opt/test/results', 'results')

def coverage_task(machine: tap.Machine, branch: str):
    try:
        if machine.run('which', 'apt-get', return_exit_code=True) == 0:
            package_install(machine, 'python3.7-dev')
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
            machine.run('python3', machine.inputs.test_scripts / 'RobotFramework.py',
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
            system_test=base_build / 'sspl-base/system_test',
            openssl=base_build / 'sspl-base' / 'openssl',
            websocket_server=context.artifact.from_component('liveterminal', 'prod', '1-0-267/219514') / 'websocket_server'
        )
    if mode == 'debug':
        test_inputs = dict(
            test_scripts=context.artifact.from_folder('./testUtils'),
            base_sdds=base_build / 'sspl-base-debug/SDDS-COMPONENT',
            system_test=base_build / 'sspl-base-debug/system_test',
            openssl=base_build / 'sspl-base-debug' / 'openssl',
            websocket_server=context.artifact.from_component('liveterminal', 'prod', '1-0-267/219514') / 'websocket_server'
        )
    if mode == 'coverage':
        test_inputs = dict(
        test_scripts=context.artifact.from_folder('./testUtils'),
        base_sdds=base_build / 'sspl-base-coverage/SDDS-COMPONENT',
        system_test=base_build / 'sspl-base-coverage/system_test',
        openssl=base_build / 'sspl-base-coverage' / 'openssl',
        websocket_server=context.artifact.from_component('liveterminal', 'prod', '1-0-267/219514') / 'websocket_server',
        bullseye_files=context.artifact.from_folder('./build/bullseye'),
        coverage=base_build / 'sspl-base-coverage/covfile',
        coverage_unittest=base_build / 'sspl-base-coverage/unittest-htmlreport',
        bazel_tools=unified_artifact(context, 'em.esg', 'develop', 'build/bazel-tools')
    )
    return test_inputs

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

@tap.pipeline(version=1, component=COMPONENT, root_sequential=False)
def sspl_base(stage: tap.Root, context: tap.PipelineContext, parameters: tap.Parameters):
    component = tap.Component(name=COMPONENT, base_version=get_base_version())
    # For cmdline/local builds, determine build mode by how tap was called
    # Not sure it's possible to pass parameters into tap so doing this for now.
    determined_build_mode=None
    if f"{component.name}.build.{DEBUG_MODE}" in sys.argv:
        determined_build_mode = DEBUG_MODE
    elif f"{component.name}.build.{RELEASE_MODE}" in sys.argv:
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
    else:
        base_build = context.artifact.build()

    # Modes where we do not want to run TAP tests.
    if mode in [ANALYSIS_MODE]:
        return

    test_inputs = get_inputs(context, base_build, mode)
    machines = (
        ("ubuntu1804",
         tap.Machine('ubuntu1804_x64_server_en_us', inputs=test_inputs,
                     platform=tap.Platform.Linux)),
        ("centos77",
         tap.Machine('centos77_x64_server_en_us', inputs=test_inputs, platform=tap.Platform.Linux)),

        ("centos82",
         tap.Machine('centos82_x64_server_en_us', inputs=test_inputs, platform=tap.Platform.Linux)),
        # add other distros here
    )

    with stage.parallel('integration'):
        task_func = robot_task
        if mode == COVERAGE_MODE:
            stage.task(task_name="centos77", func=coverage_task, machine=tap.Machine('centos77_x64_server_en_us', inputs=test_inputs, platform=tap.Platform.Linux), branch=context.branch)
        else:
            for template_name, machine in machines:
                stage.task(task_name=template_name, func=task_func, machine=machine)

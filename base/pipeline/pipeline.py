import os
import time
import tap.v1 as tap
from tap._pipeline.tasks import ArtisanInput

import requests


SYSTEM_TEST_BULLSEYE_JENKINS_JOB_URL = 'https://sspljenkins.eng.sophos/job/SSPL-Base-bullseye-system-test-coverage/build?token=sspl-linuxdarwin-coverage-token'

COVFILE_UNITTEST = '/opt/test/inputs/coverage/sspl-base-unittest.cov'
COVFILE_TAPTESTS = '/opt/test/inputs/coverage/sspl-base-taptests.cov'
UPLOAD_SCRIPT = '/opt/test/inputs/bullseye_files/uploadResults.sh'

RESULTS_DIR = '/opt/test/results'
INPUTS_DIR = '/opt/test/inputs'

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
        machine.run('python3', machine.inputs.test_scripts / 'RobotFramework.py',
                    timeout=3600)
    finally:
        machine.run('python3', machine.inputs.test_scripts / 'move_robot_results.py')
        machine.output_artifact('/opt/test/logs', 'logs')
        machine.output_artifact('/opt/test/results', 'results')


def coverage_task(machine: tap.Machine):
    try:
        if machine.run('which', 'apt-get', return_exit_code=True) == 0:
            package_install(machine, 'python3.7-dev')
        install_requirements(machine)

        # upload unit test coverage-html and the unit-tests cov file to allegro
        unitest_htmldir = os.path.join(INPUTS_DIR, "sspl-base-unittest")
        machine.run('mv', str(machine.inputs.coverage_unittest), unitest_htmldir)
        machine.run('cp', COVFILE_UNITTEST, unitest_htmldir)
        machine.run('bash', '-x', UPLOAD_SCRIPT, environment={'UPLOAD_ONLY': 'UPLOAD', 'htmldir': unitest_htmldir})

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
                        environment={'COVFILE': COVFILE_TAPTESTS})
        finally:
            machine.run('python3', machine.inputs.test_scripts / 'move_robot_results.py')

        # generate tap (tap tests + unit tests) coverage html results and upload to allegro (and the .cov file which is in tap_htmldir)
        tap_htmldir = os.path.join(INPUTS_DIR, 'sspl-base-taptest')
        machine.run ('mkdir', tap_htmldir)
        machine.run('cp', COVFILE_TAPTESTS, tap_htmldir)
        machine.run('bash', '-x', UPLOAD_SCRIPT,
                    environment={'COVFILE': COVFILE_TAPTESTS, 'BULLSEYE_UPLOAD': '1', 'htmldir': tap_htmldir})

        # publish tap (tap tests + unit tests) html results and coverage file to artifactory
        machine.run('mv', tap_htmldir, coverage_results_dir)
        machine.run('cp', COVFILE_TAPTESTS, coverage_results_dir)

        #start systemtest coverage in jenkins (these include tap-tests)
        run_sys = requests.get(url=SYSTEM_TEST_BULLSEYE_JENKINS_JOB_URL, verify=False)

    finally:
        machine.output_artifact('/opt/test/results', 'results')
        machine.output_artifact('/opt/test/logs', 'logs')


def pytest_task(machine: tap.Machine):
    try:
        install_requirements(machine)
        tests_dir = str(machine.inputs.test_scripts)
        args = ['python3', '-u', '-m', 'pytest', tests_dir,
                '-o', 'log_cli=true',
                '--html=/opt/test/results/report.html'
                ]
        machine.run(*args)
        machine.run('ls', '/opt/test/logs')
    finally:
        machine.output_artifact('/opt/test/results', 'results')
        machine.output_artifact('/opt/test/logs', 'logs')


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
    if mode == 'coverage':
        test_inputs = dict(
        test_scripts=context.artifact.from_folder('./testUtils'),
        base_sdds=base_build / 'sspl-base-coverage/SDDS-COMPONENT',
        system_test=base_build / 'sspl-base-coverage/system_test',
        openssl=base_build / 'sspl-base-coverage' / 'openssl',
        websocket_server=context.artifact.from_component('liveterminal', 'prod', '1-0-267/219514') / 'websocket_server',
        bullseye_files=context.artifact.from_folder('./build/bullseye'),
        coverage=base_build / 'sspl-base-coverage/covfile',
        coverage_unittest=base_build / 'sspl-base-coverage/unittest-htmlreport'
    )
    return test_inputs


@tap.pipeline(version=1, component='sspl-base', root_sequential=False)
def sspl_base(stage: tap.Root, context: tap.PipelineContext, parameters: tap.Parameters):
    component = tap.Component(name='sspl-base', base_version='1.1.6')
    
    RELEASE_MODE = 'release'
    ANALYSIS_MODE = 'analysis'
    COVERAGE_MODE = 'coverage'
    NINE_NINE_NINE_MODE = '999'
    ZERO_SIX_ZERO_MODE = '060'
    # export TAP_PARAMETER_MODE=release|analysis
    mode = parameters.mode or RELEASE_MODE
    base_build = None
    INCLUDE_BUILD_IN_PIPELINE = parameters.get('INCLUDE_BUILD_IN_PIPELINE', True)
    if INCLUDE_BUILD_IN_PIPELINE:
        with stage.parallel('build'):
            if mode == RELEASE_MODE or mode == ANALYSIS_MODE:
                analysis_build = stage.artisan_build(name=ANALYSIS_MODE, component=component, image='JenkinsLinuxTemplate5',
                                                 mode=ANALYSIS_MODE, release_package='./build/release-package.xml')
                base_build = stage.artisan_build(name=RELEASE_MODE, component=component, image='JenkinsLinuxTemplate5',
                                                 mode=RELEASE_MODE, release_package='./build/release-package.xml')
                nine_nine_nine_build = stage.artisan_build(name=NINE_NINE_NINE_MODE, component=component, image='JenkinsLinuxTemplate5',
                                                           mode=NINE_NINE_NINE_MODE, release_package='./build/release-package.xml')
                zero_siz_zero_build = stage.artisan_build(name=ZERO_SIX_ZERO_MODE, component=component, image='JenkinsLinuxTemplate5',
                                                           mode=ZERO_SIX_ZERO_MODE, release_package='./build/release-package.xml')
            elif mode == COVERAGE_MODE:
                base_build = stage.artisan_build(name=COVERAGE_MODE, component=component, image='JenkinsLinuxTemplate5',
                                             mode=COVERAGE_MODE, release_package='./build/release-package.xml')
                release_build = stage.artisan_build(name=RELEASE_MODE, component=component, image='JenkinsLinuxTemplate5',
                                                    mode=RELEASE_MODE, release_package='./build/release-package.xml')
                nine_nine_nine_build = stage.artisan_build(name=NINE_NINE_NINE_MODE, component=component, image='JenkinsLinuxTemplate5',
                                                           mode=NINE_NINE_NINE_MODE, release_package='./build/release-package.xml')
                zero_siz_zero_build = stage.artisan_build(name=ZERO_SIX_ZERO_MODE, component=component, image='JenkinsLinuxTemplate5',
                                                          mode=ZERO_SIX_ZERO_MODE, release_package='./build/release-package.xml')
    else:
        base_build = context.artifact.build()

    if mode == ANALYSIS_MODE:
        return

    # test_inputs = get_inputs(context, base_build, mode)
    # machines = (
    #     ("ubuntu1804",
    #      tap.Machine('ubuntu1804_x64_server_en_us', inputs=test_inputs,
    #                  platform=tap.Platform.Linux)),
    #     ("centos77",
    #      tap.Machine('centos77_x64_server_en_us', inputs=test_inputs, platform=tap.Platform.Linux)),
    #
    #     ("centos82",
    #      tap.Machine('centos82_x64_server_en_us', inputs=test_inputs, platform=tap.Platform.Linux)),
    #     # add other distros here
    # )
    # with stage.parallel('integration'):
    #     task_func = robot_task
    #     if mode == COVERAGE_MODE:
    #         stage.task(task_name="centos77", func=coverage_task, machine=tap.Machine('centos77_x64_server_en_us', inputs=test_inputs, platform=tap.Platform.Linux))
    #     else:
    #         for template_name, machine in machines:
    #             stage.task(task_name=template_name, func=task_func, machine=machine)


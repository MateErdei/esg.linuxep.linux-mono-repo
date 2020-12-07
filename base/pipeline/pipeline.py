import tap.v1 as tap

import os

from tap._pipeline.tasks import ArtisanInput

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
        machine.run('apt-get', '-y', 'install', *install_args,
                    log_mode=tap.LoggingMode.ON_ERROR)
    else:
        machine.run('yum', '-y', 'install', *install_args,
                    log_mode=tap.LoggingMode.ON_ERROR)



def install_requirements(machine: tap.Machine):
    """ install python lib requirements """
    try:
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

        # upload unit test coverage html results to allegro
        unitest_htmldir = os.path.join(INPUTS_DIR, "sspl-base-unittest")
        machine.run('mv', str(machine.inputs.coverage_unittest), unitest_htmldir)
        machine.run('bash', '-x', UPLOAD_SCRIPT, environment={'UPLOAD_ONLY': 'UPLOAD', 'htmldir': unitest_htmldir})

        # publish unit test coverage file and results to artifactory results/coverage
        coverage_results_dir = os.path.join(RESULTS_DIR, 'coverage')
        machine.run('rm', '-rf', coverage_results_dir)
        machine.run('mkdir', coverage_results_dir)
        machine.run('cp', "-r", unitest_htmldir, coverage_results_dir)
        machine.run('cp', COVFILE_UNITTEST, coverage_results_dir)

        # run component pytests and integration robot tests with coverage file to get combined coverage
        machine.run('mv', COVFILE_UNITTEST, COVFILE_TAPTESTS)

        # Run component pytest
        # These are disabled for now
        try:
            machine.run('python3', machine.inputs.test_scripts / 'RobotFramework.py', timeout=3600,
                        environment={'COVFILE': COVFILE_TAPTESTS})
        finally:
            machine.run('python3', machine.inputs.test_scripts / 'move_robot_results.py')


        # generate combined coverage html results and upload to allegro
        taptest_htmldir = os.path.join(INPUTS_DIR, 'edr', 'coverage', 'sspl-base-taptests')
        machine.run('bash', '-x', UPLOAD_SCRIPT,
                    environment={'COVFILE': COVFILE_TAPTESTS, 'BULLSEYE_UPLOAD': '1', 'htmldir': taptest_htmldir})

        # publish combined html results and coverage file to artifactory
        machine.run('mv', taptest_htmldir, coverage_results_dir)
        machine.run('cp', COVFILE_TAPTESTS, coverage_results_dir)
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
        base_sdds=base_build / 'sspl-base/SDDS-COMPONENT',
        system_test=base_build / 'sspl-base/system_test',
        openssl=base_build / 'sspl-base' / 'openssl',
        websocket_server=context.artifact.from_component('liveterminal', 'prod', '1-0-267/219514') / 'websocket_server',
        bullseye_files=context.artifact.from_folder('./build/bullseye'),
        coverage=base_build / 'coverage',
        coverage_unittest=base_build / 'coverage/unittest'
    )
    return test_inputs


@tap.pipeline(version=1, component='sspl-base', root_sequential=False)
def sspl_base(stage: tap.Root, context: tap.PipelineContext, parameters: tap.Parameters):
    component = tap.Component(name='sspl-base', base_version='1.1.4')

    release_mode = 'release'
    analysis_mode = 'analysis'

    # export TAP_PARAMETER_MODE=release|analysis
    mode = parameters.mode or release_mode
    base_build = None
    INCLUDE_BUILD_IN_PIPELINE = parameters.get('INCLUDE_BUILD_IN_PIPELINE', True)
    if INCLUDE_BUILD_IN_PIPELINE:
        with stage.parallel('build'):
            if mode == release_mode or mode == analysis_mode:
                base_build = stage.artisan_build(name=release_mode,
                                                 component=component,
                                                 image='JenkinsLinuxTemplate5',
                                                 mode=release_mode,
                                                 release_package='./build/release-package.xml')

                analysis_build = stage.artisan_build(name=analysis_mode,
                                                     component=component,
                                                     image='JenkinsLinuxTemplate5',
                                                     mode=analysis_mode,
                                                     release_package='./build/release-package.xml')
            else:
                base_build = stage.artisan_build(name=mode,
                                                 component=component,
                                                 image='JenkinsLinuxTemplate5',
                                                 mode=mode,
                                                 release_package='./build/release-package.xml')
    else:
        base_build = context.artifact.build()

    if mode == 'analysis':
        return

    test_inputs = get_inputs(context, base_build, mode)
    machines = (
        ("ubuntu1804",
         tap.Machine('ubuntu1804_x64_server_en_us', inputs=test_inputs,
                     platform=tap.Platform.Linux)),
        ("centos77",
         tap.Machine('centos77_x64_server_en_us', inputs=test_inputs, platform=tap.Platform.Linux))
        # add other distros here
    )
    with stage.parallel('integration'):
        for template_name, machine in machines:
            stage.task(task_name=template_name, func=robot_task, machine=machine)

    # with stage.group('component'):
    #     stage.task(task_name='ubuntu1804_x64', func=pytest_task, machine=machine)

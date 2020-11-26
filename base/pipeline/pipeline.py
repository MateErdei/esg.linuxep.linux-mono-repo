import tap.v1 as tap

import os

from tap._pipeline.tasks import ArtisanInput


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
    return test_inputs


@tap.pipeline(version=1, component='sspl-base', root_sequential=False)
def sspl_base(stage: tap.Root, context: tap.PipelineContext, parameters: tap.Parameters):
    component = tap.Component(name='sspl-base', base_version='1.1.4')

    # export TAP_PARAMETER_MODE=release|analysis
    mode = parameters.get('MODE', 'release')
    base_build = None
    INCLUDE_BUILD_IN_PIPELINE = parameters.get('INCLUDE_BUILD_IN_PIPELINE', True)
    if INCLUDE_BUILD_IN_PIPELINE:
        with stage.parallel('build'):
            base_build = stage.artisan_build(name=mode, component=component, image='JenkinsLinuxTemplate5',
                                             mode=mode, release_package='./build/release-package.xml')
    else:
        base_build = context.artifact.build()


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

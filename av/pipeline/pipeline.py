import tap.v1 as tap

import os


def pip_install(machine: tap.Machine, *install_args: str):
    """Installs python packages onto a TAP machine"""
    pip_index = os.environ.get('TAP_PIP_INDEX_URL')
    pip_index_args = ['--index-url', pip_index] if pip_index else []
    pip_index_args += ["--no-cache-dir",
                       "--progress-bar", "off",
                       "--disable-pip-version-check",
                       "--default-timeout", "120"]
    machine.run('pip', '--log', '/opt/test/logs/pip.log',
                'install', *install_args, *pip_index_args,
                log_mode=tap.LoggingMode.ON_ERROR)


def package_install(machine: tap.Machine, *install_args: str):
    machine.run('apt-get', '-y', 'install', *install_args,
                log_mode=tap.LoggingMode.ON_ERROR)


def has_coverage_build(branch_name):
    """If the branch name does an analysis mode build"""
    return branch_name in (
                           'bugfix/get_coverage_builds_working'
                           ) or branch_name.endswith('coverage')


def install_requirements(machine: tap.Machine):
    """ install python lib requirements """
    package_install(machine, 'python3.7-dev')
    pip_install(machine, '-r', machine.inputs.test_scripts / 'requirements.txt')
    try:
        machine.run('useradd', 'sophos-spl-user')
        machine.run('groupadd', 'sophos-spl-group')
    except Exception as ex:
        # the previous command will fail if user already exists. But this is not an error
        print("On adding user and group: {}".format(ex))


def robot_task(machine: tap.Machine):
    try:
        package_install(machine, 'nfs-kernel-server')
        install_requirements(machine)
        machine.run('python', machine.inputs.test_scripts / 'RobotFramework.py')
    finally:
        machine.run('python', machine.inputs.test_scripts / 'move_robot_results.py')
        machine.output_artifact('/opt/test/logs', 'logs')
        machine.output_artifact('/opt/test/results', 'results')


def pytest_task(machine: tap.Machine):
    try:
        install_requirements(machine)
        tests_dir = str(machine.inputs.test_scripts)
        args = ['python', '-u', '-m', 'pytest', tests_dir,
                '-o', 'log_cli=true',
                '--html=/opt/test/results/report.html'
                ]
        machine.run(*args)
        machine.run('ls', '/opt/test/logs')
    finally:
        machine.output_artifact('/opt/test/results', 'results')
        machine.output_artifact('/opt/test/logs', 'logs')


def get_inputs(context: tap.PipelineContext, coverage=False):
    print(str(context.artifact.build()))
    test_inputs = dict(
        test_scripts=context.artifact.from_folder('./TA'),
        av=context.artifact.build() / 'output'
    )
    # override the av input and get the bullseye coverage build instead
    if coverage:
        assert has_coverage_build(context.branch)
        test_inputs['av'] = context.artifact.build() / 'coverage'

    return test_inputs


@tap.pipeline(version=1, component='sspl-plugin-anti-virus')
def av_plugin(stage: tap.Root, context: tap.PipelineContext):
    machine = tap.Machine('ubuntu1804_x64_server_en_us', inputs=get_inputs(context), platform=tap.Platform.Linux)
    has_coverage = has_coverage_build(context.branch)
    if has_coverage:
        machine_bullseye_test = tap.Machine('ubuntu1804_x64_server_en_us',
                                            inputs=get_inputs(context,
                                                              coverage=True),
                                            platform=tap.Platform.Linux)
    else:
        machine_bullseye_test = None

    with stage.group('component'):
        stage.task(task_name='ubuntu1804_x64', func=pytest_task, machine=machine)
        if has_coverage:
            stage.task(task_name='ubuntu1804_x64_coverage', func=pytest_task, machine=machine_bullseye_test)

    with stage.group('integration'):
        stage.task(task_name='ubuntu1804_x64', func=robot_task, machine=machine)

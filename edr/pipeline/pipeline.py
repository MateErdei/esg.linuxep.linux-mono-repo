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

def install_requirements(machine: tap.Machine):
    """ install python lib requirements """
    pip_install(machine, '-r', machine.inputs.test_scripts / 'requirements.txt')

def robot_task(machine: tap.Machine):
    try:
        install_requirements(machine)
        machine.run('python', machine.inputs.test_scripts / 'RobotFramework.py')
    finally:
        machine.run('python', machine.inputs.test_scripts / 'move_robot_results.py')
        machine.output_artifact('/opt/test/logs', 'logs')
        machine.output_artifact('/opt/test/results', 'results')


def get_inputs(context: tap.PipelineContext):
    print(str(context.artifact.build()))
    test_inputs = dict(
        test_scripts=context.artifact.from_folder('./TA'),
        system_producttest_output=context.artifact.from_component('sspl-base', '1-0-1-451') / 'SystemProductTestOutput.tar.gz',
        edr=context.artifact.build() / 'output'
    )
    return test_inputs


@tap.pipeline(version=1, component='sspl-edr-plugin')
def edr_plugin(stage: tap.Root, context: tap.PipelineContext):
    machine=tap.Machine('ubuntu1804_x64_server_en_us', inputs=get_inputs(context), platform=tap.Platform.Linux)
    with stage.group('test'):
        stage.task(task_name='ubuntu1804_x64', func=robot_task, machine=machine)


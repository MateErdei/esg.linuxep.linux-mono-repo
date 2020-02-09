import tap.v1 as tap

import os

COVFILE_UNITTEST = '/opt/test/inputs/edr/sspl-plugin-edr-unit.cov'
COVFILE_COMBINED = '/opt/test/logs/sspl-edr-combined.cov'
UPLOAD_SCRIPT = '/opt/test/inputs/edr/bullseye/uploadResults.sh'
LOGS_DIR = '/opt/testlogs'
INPUTS_DIR = '/opt/test/inputs'

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


def has_coverage_build(branch_name):
    """If the branch name does an analysis mode build"""
    return branch_name == 'master' or branch_name.endswith('ci-and-tap')


def install_requirements(machine: tap.Machine):
    """ install python lib requirements """
    pip_install(machine, '-r', machine.inputs.test_scripts / 'requirements.txt')
    try:
        machine.run('useradd', 'sophos-spl-user')
        machine.run('groupadd', 'sophos-spl-group')
    except Exception as ex:
        # the previous command will fail if user already exists. But this is not an error
        print("On adding user and group: {}".format(ex))


def robot_task(machine: tap.Machine):
    try:
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

        if has_coverage_build(context.branch):
            #upload unit test coverage
            unitest_htmldir = os.path.join(INPUTS_DIR, 'edr', 'coverage', 'sspl-plugin-edr-unittest')
            machine.run('bash', 'x',  UPLOAD_SCRIPT, environment={'UPLOAD_ONLY': 'UPLOAD', 'htmldir': unitest_htmldir})
            machine.run('mv', unitest_htmldir, LOGS_DIR)
            machine.run('cp', COVFILE_UNITTEST, LOGS_DIR)
            
            #run component tests with coverage file
            machine.run('mv', COVFILE_UNITTEST, COVFILE_COMBINED)
            machine.run(*args, environment={'COVFILE': COVFILE_COMBINED})

            combined_htmldir = os.path.join(INPUTS_DIR, 'edr', 'coverage', 'sspl-plugin-edr-combined')
            machine.run('bash', 'x',  UPLOAD_SCRIPT, environment={'BULLSEYE_UPLOAD': 1, 'htmldir': combined_htmldir})
        else:
            machine.run(*args)

        machine.run('ls', '/opt/test/logs')
        machine.run('ls', '/opt/test/inputs/bullseye_files')
    finally:
        machine.output_artifact('/opt/test/results', 'results')
        machine.output_artifact('/opt/test/logs', 'logs')


def get_inputs(context: tap.PipelineContext, coverage=False):
    print(str(context.artifact.build()))
    test_inputs = dict(
        test_scripts=context.artifact.from_folder('./TA'),
        edr=context.artifact.build() / 'output'
    )
    #override the edr input and get the bullseye coverage build insteady
    if coverage and has_coverage_build(context.branch):
        test_inputs['edr'] = context.artifact.build() / 'coverage'
        test_inputs['bullseye_files'] = context.artifact.from_folder('./build/bullseye')

    return test_inputs


@tap.pipeline(version=1, component='sspl-edr-plugin')
def edr_plugin(stage: tap.Root, context: tap.PipelineContext):
    machine=tap.Machine('ubuntu1804_x64_server_en_us', inputs=get_inputs(context), platform=tap.Platform.Linux)
    machine_bullseye_test=tap.Machine('ubuntu1804_x64_server_en_us', inputs=get_inputs(context, coverage=True), platform=tap.Platform.Linux)
    with stage.group('integration'):
        stage.task(task_name='ubuntu1804_x64', func=robot_task, machine=machine)
    with stage.group('component'):
        stage.task(task_name='ubuntu1804_x64', func=pytest_task, machine=machine)
        if has_coverage_build(context.branch):
            stage.task(task_name='ubuntu1804_x64_coverage', func=pytest_task, machine=machine_bullseye_test)


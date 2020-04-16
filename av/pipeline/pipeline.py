import logging
import os

import tap.v1 as tap

COVFILE_UNITTEST = '/opt/test/inputs/av/sspl-plugin-av-unit.cov'
COVFILE_COMBINED = '/opt/test/inputs/av/sspl-av-combined.cov'
UPLOAD_SCRIPT = '/opt/test/inputs/bullseye_files/uploadResults.sh'
LOGS_DIR = '/opt/test/logs'
RESULTS_DIR = '/opt/test/results'
INPUTS_DIR = '/opt/test/inputs'

logger = logging.getLogger(__name__)


def has_coverage_build(branch_name):
    """If the branch name does an analysis mode build"""
    return branch_name == 'master' or branch_name.endswith('coverage')


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


def robot_task(machine: tap.Machine, environment=None):
    try:
        package_install(machine, 'nfs-kernel-server')
        install_requirements(machine)
        machine.run('python', machine.inputs.test_scripts / 'RobotFramework.py', environment=environment)
    finally:
        machine.run('python', machine.inputs.test_scripts / 'move_robot_results.py')
        machine.output_artifact('/opt/test/logs', 'logs')
        machine.output_artifact('/opt/test/results', 'results')


def pytest_task(machine: tap.Machine, environment=None):
    try:
        install_requirements(machine)
        tests_dir = str(machine.inputs.test_scripts)
        args = ['python', '-u', '-m', 'pytest', tests_dir,
                '-o', 'log_cli=true',
                '--html=/opt/test/results/report.html'
                ]
        machine.run(*args, environment=environment)
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


def bullseye_coverage_task(machine: tap.Machine):
    try:
        install_requirements(machine)

        # upload unit test coverage html results to allegro
        unitest_htmldir = os.path.join(INPUTS_DIR, 'av', 'coverage', 'sspl-plugin-av-unittest')
        machine.run('bash', '-x', UPLOAD_SCRIPT, environment={'UPLOAD_ONLY': 'UPLOAD', 'htmldir': unitest_htmldir})

        # publish unit test coverage file and results to artifactory results/coverage
        coverage_results_dir = os.path.join(RESULTS_DIR, 'coverage')
        machine.run('rm', '-rf', coverage_results_dir)
        machine.run('mkdir', coverage_results_dir)
        machine.run('mv', unitest_htmldir, coverage_results_dir)
        machine.run('cp', COVFILE_UNITTEST, coverage_results_dir)

        machine.run('mv', COVFILE_UNITTEST, COVFILE_COMBINED)

        # run component pytests and integration robot tests with coverage file to get combined coverage
        pytest_task(machine, environment={'COVFILE': COVFILE_COMBINED})
        robot_task(machine, environment={'COVFILE': COVFILE_COMBINED})

        # generate combined coverage html results and upload to allegro
        combined_htmldir = os.path.join(INPUTS_DIR, 'av', 'coverage', 'sspl-plugin-av-combined')
        machine.run('bash', '-x', UPLOAD_SCRIPT,
                    environment={'COVFILE': COVFILE_COMBINED, 'BULLSEYE_UPLOAD': '1', 'htmldir': combined_htmldir})

        # publish combined html results and coverage file to artifactory
        machine.run('mv', combined_htmldir, coverage_results_dir)
        machine.run('cp', COVFILE_COMBINED, coverage_results_dir)
    finally:
        machine.output_artifact('/opt/test/results', 'results')
        machine.output_artifact('/opt/test/logs', 'logs')


@tap.pipeline(version=1, component='sspl-plugin-anti-virus')
def av_plugin(stage: tap.Root, context: tap.PipelineContext, parameters: tap.Parameters):

    machine = tap.Machine('ubuntu1804_x64_server_en_us', inputs=get_inputs(context), platform=tap.Platform.Linux)

    with stage.group('component'):
        stage.task(task_name='ubuntu1804_x64', func=pytest_task, machine=machine)

    with stage.group('integration'):
        stage.task(task_name='ubuntu1804_x64', func=robot_task, machine=machine)

    with stage.group('coverage'):
        branch_name = context.branch
        if parameters.coverage == 'yes' or has_coverage_build(branch_name):
            machine_bullseye_test = tap.Machine('ubuntu1804_x64_server_en_us',
                                                inputs=get_inputs(context, coverage=True),
                                                platform=tap.Platform.Linux)
            stage.task(task_name='ubuntu1804_x64_combined', func=bullseye_coverage_task, machine=machine_bullseye_test)

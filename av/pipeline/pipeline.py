import logging
import os

import tap.v1 as tap

COVFILE_UNITTEST = '/opt/test/inputs/av/sspl-plugin-av-unit.cov'
COVFILE_PYTEST = '/opt/test/inputs/av/sspl-plugin-av-pytest.cov'
COVFILE_ROBOT = '/sspl-plugin-av-robot.cov' ## Move to root, so that everyone can access it
COVFILE_COMBINED = '/opt/test/inputs/av/sspl-plugin-av-combined.cov'
BULLSEYE_SCRIPT_DIR = '/opt/test/inputs/bullseye_files'
UPLOAD_SCRIPT = os.path.join(BULLSEYE_SCRIPT_DIR, 'uploadResults.sh')
UPLOAD_ROBOT_LOG_SCRIPT = os.path.join(BULLSEYE_SCRIPT_DIR, 'uploadRobotLog.sh')
LOGS_DIR = '/opt/test/logs'
RESULTS_DIR = '/opt/test/results'
INPUTS_DIR = '/opt/test/inputs'
COVSRCDIR = os.path.join(INPUTS_DIR, 'av', 'src')

logger = logging.getLogger(__name__)

BRANCH_NAME = "master"

def has_coverage_build(branch_name):
    """If the branch name does an analysis mode build"""
    return branch_name == 'master' or branch_name.endswith('coverage')

def is_debian_based(machine: tap.Machine):
    return machine.template.startswith("ubuntu")

def is_redhat_based(machine: tap.Machine):
    return machine.template.startswith("centos")


def pip(machine: tap.Machine):
    return "pip3"


def python(machine: tap.Machine):
    return "python3"


def pip_install(machine: tap.Machine, *install_args: str):
    """Installs python packages onto a TAP machine"""
    pip_index = os.environ.get('TAP_PIP_INDEX_URL')
    pip_index_args = ['--index-url', pip_index] if pip_index else []
    pip_index_args += ["--no-cache-dir",
                       "--progress-bar", "off",
                       "--disable-pip-version-check",
                       "--default-timeout", "120"]
    machine.run(pip(machine), '--log', '/opt/test/logs/pip.log',
                'install', '-v', *install_args, *pip_index_args,
                log_mode=tap.LoggingMode.ON_ERROR)


def install_requirements(machine: tap.Machine):
    """ install python lib requirements """
    machine.run('bash', machine.inputs.test_scripts / "bin/install_pip_prerequisites.sh")
    pip_install(machine, '-r', machine.inputs.test_scripts / 'requirements.txt')
    try:
        machine.run('useradd', 'sophos-spl-user')
        machine.run('groupadd', 'sophos-spl-group')
    except Exception as ex:
        # the previous command will fail if user already exists. But this is not an error
        print("On adding user and group: {}".format(ex))


def get_suffix():
    global BRANCH_NAME
    if BRANCH_NAME == "master":
        return ""
    return "-" + BRANCH_NAME


def robot_task_with_env(machine: tap.Machine, environment=None):
    try:
        machine.run('bash', machine.inputs.test_scripts / "bin/install_nfs_server.sh")
        machine.run(python(machine), machine.inputs.test_scripts / 'RobotFramework.py', environment=environment)
    finally:
        machine.run(python(machine), machine.inputs.test_scripts / 'move_robot_results.py')
        machine.output_artifact('/opt/test/logs', 'logs')
        machine.output_artifact('/opt/test/results', 'results')
        machine.run('bash', UPLOAD_ROBOT_LOG_SCRIPT, "/opt/test/logs/log.html", "robot-log"+get_suffix()+".html")


def robot_task(machine: tap.Machine):
    install_requirements(machine)
    robot_task_with_env(machine)


def pytest_task_with_env(machine: tap.Machine, environment=None):
    try:
        tests_dir = str(machine.inputs.test_scripts)
        args = [python(machine), '-u', '-m', 'pytest', tests_dir,
                '-o', 'log_cli=true',
                '--html=/opt/test/results/report.html'
                ]
        machine.run(*args, environment=environment)
        machine.run('ls', '/opt/test/logs')
    finally:
        machine.output_artifact('/opt/test/results', 'results')
        machine.output_artifact('/opt/test/logs', 'logs')


def pytest_task(machine: tap.Machine):
    install_requirements(machine)
    pytest_task_with_env(machine)


def get_inputs(context: tap.PipelineContext, coverage=False):
    print(str(context.artifact.build()))
    test_inputs = dict(
        test_scripts=context.artifact.from_folder('./TA'),
        bullseye_files=context.artifact.from_folder('./build/bullseye'),  # used for robot upload
        av=context.artifact.build() / 'output'
    )
    # override the av input and get the bullseye coverage build instead
    if coverage:
        assert has_coverage_build(context.branch)
        test_inputs['av'] = context.artifact.build() / 'coverage'

    return test_inputs


def bullseye_coverage_task(machine: tap.Machine):
    suffix = get_suffix()

    try:
        exception = None

        install_requirements(machine)

        coverage_results_dir = os.path.join(RESULTS_DIR, 'coverage')
        machine.run('rm', '-rf', coverage_results_dir)
        machine.run('mkdir', coverage_results_dir)

        # upload unit test coverage html results to allegro
        unitest_htmldir = os.path.join(INPUTS_DIR, 'av', 'coverage_html')
        machine.run('bash', UPLOAD_SCRIPT,
                    environment={
                        'COV_HTML_BASE': 'sspl-plugin-av-unittest' + suffix,
                        'UPLOAD_ONLY': 'UPLOAD',
                        'htmldir': unitest_htmldir,
                    })
        # publish unit test coverage file and results to artifactory results/coverage
        machine.run('mv', unitest_htmldir, coverage_results_dir)
        machine.run('cp', COVFILE_UNITTEST, coverage_results_dir)

        machine.run('cp', COVFILE_UNITTEST, COVFILE_PYTEST)
        machine.run('covclear', environment={
            'COVFILE': COVFILE_PYTEST,
            'COVSRCDIR': COVSRCDIR,
        })
        # run component pytests and integration robot tests with coverage file to get combined coverage
        pytest_task_with_env(machine, environment={
            'COVFILE': COVFILE_PYTEST,
            'COVSRCDIR': COVSRCDIR,
        })
        pytest_htmldir = os.path.join(INPUTS_DIR, 'av', 'coverage', 'sspl-plugin-av-pytest')
        machine.run('bash', UPLOAD_SCRIPT,
                    environment={
                        'COVFILE': COVFILE_PYTEST,
                        'COVSRCDIR': COVSRCDIR,
                        'COV_HTML_BASE': 'sspl-plugin-av-pytest' + suffix,
                        'htmldir': pytest_htmldir,
                    })
        machine.run('mv', pytest_htmldir, coverage_results_dir)
        machine.run('cp', COVFILE_PYTEST, coverage_results_dir)

        machine.run('cp', COVFILE_UNITTEST, COVFILE_ROBOT)
        machine.run('covclear', environment={
            'COVFILE': COVFILE_ROBOT,
            'COVSRCDIR': COVSRCDIR,
        })

        machine.run('chmod', '666', COVFILE_ROBOT)
        # set bullseye environment in a file, so that daemons pick up the settings too
        machine.run('bash', os.path.join(BULLSEYE_SCRIPT_DIR, "createBullseyeCoverageEnv.sh"),
                    COVFILE_ROBOT, COVSRCDIR)

        # don't abort immediately if robot tests fail, generate the coverage report, then re-raise the exception
        try:
            robot_task_with_env(machine, environment={
                'COVFILE': COVFILE_ROBOT,
                'COVSRCDIR': COVSRCDIR,
            })
        except tap.exceptions.PipelineProcessExitNonZeroError as e:
            exception = e

        machine.run('rm', '-f', '/tmp/BullseyeCoverageEnv.txt')
        robot_htmldir = os.path.join(INPUTS_DIR, 'av', 'coverage', 'sspl-plugin-av-robot')
        machine.run('bash', UPLOAD_SCRIPT,
                    environment={
                        'COVFILE': COVFILE_ROBOT,
                        'COVSRCDIR': COVSRCDIR,
                        'COV_HTML_BASE': 'sspl-plugin-av-robot' + suffix,
                        'htmldir': robot_htmldir,
                    })
        machine.run('mv', robot_htmldir, coverage_results_dir)
        machine.run('cp', COVFILE_ROBOT, coverage_results_dir)

        # generate combined coverage html results and upload to allegro
        machine.run('covmerge', '--create', '--file', COVFILE_COMBINED,
                    COVFILE_UNITTEST,
                    COVFILE_PYTEST,
                    COVFILE_ROBOT
                    )
        combined_htmldir = os.path.join(INPUTS_DIR, 'av', 'coverage', 'sspl-plugin-av-combined')
        machine.run('bash', UPLOAD_SCRIPT,
                    environment={
                        'COVFILE': COVFILE_COMBINED,
                        'COVSRCDIR': COVSRCDIR,
                        'COV_HTML_BASE': 'sspl-plugin-av-combined' + suffix,
                        'htmldir': combined_htmldir,
                    })

        # publish combined html results and coverage file to artifactory
        machine.run('mv', combined_htmldir, coverage_results_dir)
        machine.run('cp', COVFILE_COMBINED, coverage_results_dir)

        if exception is not None:
            raise exception
    finally:
        machine.output_artifact('/opt/test/results', 'results')
        machine.output_artifact('/opt/test/logs', 'logs')
        machine.run('bash', UPLOAD_ROBOT_LOG_SCRIPT, "/opt/test/logs/log.html", "robot-coverage-log"+suffix+".html")


@tap.pipeline(component='sspl-plugin-anti-virus', root_sequential=False)
def av_plugin(stage: tap.Root, context: tap.PipelineContext, parameters: tap.Parameters):
    ubuntu1804_machine = tap.Machine('ubuntu1804_x64_server_en_us', inputs=get_inputs(context), platform=tap.Platform.Linux)
    centos7_machine = tap.Machine('centos77_x64_server_en_us', inputs=get_inputs(context), platform=tap.Platform.Linux)

    global BRANCH_NAME
    BRANCH_NAME = context.branch

    with stage.parallel('component'):
        stage.task(task_name='ubuntu1804_x64', func=pytest_task, machine=ubuntu1804_machine)
        stage.task(task_name='centos77_x64',   func=pytest_task, machine=centos7_machine)

    with stage.parallel('integration'):
        stage.task(task_name='ubuntu1804_x64', func=robot_task, machine=ubuntu1804_machine)
        stage.task(task_name='centos77_x64',   func=robot_task, machine=centos7_machine)

    with stage.parallel('coverage'):
        if parameters.run_tests_on_coverage == 'yes' or has_coverage_build(BRANCH_NAME):
            machine_bullseye_test = tap.Machine('ubuntu1804_x64_server_en_us',
                                                inputs=get_inputs(context, coverage=True),
                                                platform=tap.Platform.Linux)
            stage.task(task_name='ubuntu1804_x64_combined', func=bullseye_coverage_task, machine=machine_bullseye_test)
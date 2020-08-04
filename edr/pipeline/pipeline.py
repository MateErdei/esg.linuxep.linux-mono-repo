import tap.v1 as tap

import os
import logging

logger = logging.getLogger(__name__)

COVFILE_UNITTEST = '/opt/test/inputs/edr/sspl-plugin-edr-unit.cov'
COVFILE_COMBINED = '/opt/test/inputs/edr/sspl-edr-combined.cov'
UPLOAD_SCRIPT = '/opt/test/inputs/bullseye_files/uploadResults.sh'
LOGS_DIR = '/opt/test/logs'
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
    machine.run('pip', '--log', '/opt/test/logs/pip.log',
                'install', *install_args, *pip_index_args,
                log_mode=tap.LoggingMode.ON_ERROR)


def has_coverage_build(branch_name):
    """If the branch name does an analysis mode build"""
    return branch_name == 'develop' or branch_name.endswith('coverage')

def has_coverage_file(machine: tap.Machine):
    """If the downloaded build output has a coverage file then its a bullseye build"""
    return machine.run('test', '-f', COVFILE_UNITTEST, return_exit_code=True) == 0

def install_requirements(machine: tap.Machine):
    """ install python lib requirements """
    pip_install(machine, '-r', machine.inputs.test_scripts / 'requirements.txt')
    try:
        machine.run('useradd', 'sophos-spl-user')
        machine.run('groupadd', 'sophos-spl-group')
    except Exception as ex:
        # the previous command will fail if user already exists. But this is not an error
        logger.warning("On adding user and group: {}".format(ex))


def robot_task(machine: tap.Machine):
    try:
        install_requirements(machine)
        machine.run('python3', machine.inputs.test_scripts / 'RobotFramework.py')
    finally:
        machine.run('python3', machine.inputs.test_scripts / 'move_robot_results.py')
        machine.output_artifact('/opt/test/logs', 'logs')
        machine.output_artifact('/opt/test/results', 'results')


def combined_task(machine: tap.Machine):
    try:
        install_requirements(machine)
        tests_dir = str(machine.inputs.test_scripts)

        args = ['python3', '-u', '-m', 'pytest', tests_dir, '--html=/opt/test/results/report.html']
        if not has_coverage_file(machine):
            #run pytests
            machine.run(*args)
            try:
                #run robot tests
                machine.run('python3', machine.inputs.test_scripts / 'RobotFramework.py')
            finally:
                machine.run('python3', machine.inputs.test_scripts / 'move_robot_results.py')

        else:
            # upload unit test coverage html results to allegro
            unitest_htmldir = os.path.join(INPUTS_DIR, 'edr', 'coverage', 'sspl-plugin-edr-unittest')
            machine.run('bash', '-x', UPLOAD_SCRIPT, environment={'UPLOAD_ONLY': 'UPLOAD', 'htmldir': unitest_htmldir})

            # publish unit test coverage file and results to artifactory results/coverage
            coverage_results_dir = os.path.join(RESULTS_DIR, 'coverage')
            machine.run('rm', '-rf', coverage_results_dir)
            machine.run('mkdir', coverage_results_dir)
            machine.run('mv', unitest_htmldir, coverage_results_dir)
            machine.run('cp', COVFILE_UNITTEST, coverage_results_dir)

            # run component pytests and integration robot tests with coverage file to get combined coverage
            sdds = os.path.join(INPUTS_DIR, 'edr', 'SDDS-COMPONENT')
            sdds_coverage = os.path.join(INPUTS_DIR, 'edr', 'SDDS-COMPONENT-COVERAGE')
            machine.run('rm', '-rf', sdds)
            machine.run('mv', sdds_coverage, sdds)
            machine.run('mv', COVFILE_UNITTEST, COVFILE_COMBINED)

            #run component pytest
            machine.run(*args, environment={'COVFILE': COVFILE_COMBINED})
            try:
                machine.run('python3', machine.inputs.test_scripts / 'RobotFramework.py',
                            environment={'COVFILE': COVFILE_COMBINED})
            finally:
                machine.run('python', machine.inputs.test_scripts / 'move_robot_results.py')

            # generate combined coverage html results and upload to allegro
            combined_htmldir = os.path.join(INPUTS_DIR, 'edr', 'coverage', 'sspl-plugin-edr-combined')
            machine.run('bash', '-x', UPLOAD_SCRIPT,
                        environment={'COVFILE': COVFILE_COMBINED, 'BULLSEYE_UPLOAD': '1', 'htmldir': combined_htmldir})

            # publish combined html results and coverage file to artifactory
            machine.run('mv', combined_htmldir, coverage_results_dir)
            machine.run('cp', COVFILE_COMBINED, coverage_results_dir)
    finally:
        machine.output_artifact('/opt/test/results', 'results')
        machine.output_artifact('/opt/test/logs', 'logs')


def pytest_task(machine: tap.Machine):
    try:
        install_requirements(machine)
        tests_dir = str(machine.inputs.test_scripts)

        # To run the pytest with additional verbosity add following to arguments
        # '-o', 'log_cli=true'
        args = ['python3', '-u', '-m', 'pytest', tests_dir, '--html=/opt/test/results/report.html']
        machine.run(*args)
        machine.run('ls', '/opt/test/logs')
    finally:
        machine.output_artifact('/opt/test/results', 'results')
        machine.output_artifact('/opt/test/logs', 'logs')


def get_inputs(context: tap.PipelineContext):
    logger.info(str(context.artifact.build()))
    test_inputs = dict(
        test_scripts=context.artifact.from_folder('./TA'),
        edr=context.artifact.build() / 'output',
        bullseye_files=context.artifact.from_folder('./build/bullseye')
    )
    return test_inputs


@tap.pipeline(version=1, component='sspl-plugin-edr-component')
def edr_plugin(stage: tap.Root, context: tap.PipelineContext, parameters: tap.Parameters):
    machine = tap.Machine('ubuntu1804_x64_server_en_us', inputs=get_inputs(context), platform=tap.Platform.Linux)

    if parameters.coverage == 'yes' or has_coverage_build(context.branch):
        with stage.parallel('combined'):
            stage.task(task_name='ubuntu1804_x64_combined', func=combined_task, machine=machine)
            #add other distros here
    else:
        with stage.parallel('integration'):
            stage.task(task_name='ubuntu1804_x64', func=robot_task, machine=machine)
            #add other distros here

        with stage.parallel('component'):
            stage.task(task_name='ubuntu1804_x64', func=pytest_task, machine=machine)
            #add other distros here
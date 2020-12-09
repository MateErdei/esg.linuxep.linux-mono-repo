import tap.v1 as tap

import os
import logging

logger = logging.getLogger(__name__)

COVFILE_UNITTEST = '/opt/test/inputs/coverage/sspl-plugin-edr-unit.cov'
COVFILE_COMBINED = '/opt/test/inputs/coverage/sspl-edr-combined.cov'
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
    machine.run('pip3', '--log', '/opt/test/logs/pip.log',
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
        machine.run('useradd', 'sophos-spl-local')
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



        # upload unit test coverage html results to allegro
        unitest_htmldir = os.path.join(INPUTS_DIR, "sspl-plugin-edr-unittest")
        machine.run('mv', str(machine.inputs.coverage_unittest), unitest_htmldir)
        machine.run('bash', '-x', UPLOAD_SCRIPT, environment={'UPLOAD_ONLY': 'UPLOAD', 'htmldir': unitest_htmldir})


        # publish unit test coverage file and results to artifactory results/coverage
        coverage_results_dir = os.path.join(RESULTS_DIR, 'coverage')
        machine.run('rm', '-rf', coverage_results_dir)
        machine.run('mkdir', coverage_results_dir)
        machine.run('cp', "-r", unitest_htmldir, coverage_results_dir)
        machine.run('cp', COVFILE_UNITTEST, coverage_results_dir)

        # run component pytests and integration robot tests with coverage file to get combined coverage
        machine.run('mv', COVFILE_UNITTEST, COVFILE_COMBINED)

        # run component pytest
        args = ['python3', '-u', '-m', 'pytest', tests_dir, '--html=/opt/test/results/report.html']
        machine.run(*args, environment={'COVFILE': COVFILE_COMBINED})
        try:
            machine.run('python3', machine.inputs.test_scripts / 'RobotFramework.py',
                        environment={'COVFILE': COVFILE_COMBINED})
        finally:
            machine.run('python3', machine.inputs.test_scripts / 'move_robot_results.py')

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

def unified_artifact(context: tap.PipelineContext, component: str, branch: str, sub_directory: str):
    """Return an input artifact from an unified pipeline build"""
    artifact = context.artifact.from_component(component, branch, org='', storage='esg-build-tested')
    # Using the truediv operator to set the sub directory forgets the storage option
    artifact.sub_directory = sub_directory
    return artifact


def get_inputs(context: tap.PipelineContext, edr_build, mode: str):
    test_inputs = None
    if mode == 'release':
        test_inputs = dict(
            test_scripts=context.artifact.from_folder('./TA'),
            edr_sdds=edr_build / 'edr/SDDS-COMPONENT',
            base_sdds=edr_build / 'base/base-sdds',
            componenttests=edr_build / 'componenttests',
            qp=unified_artifact(context, 'em.esg', 'develop', 'build/scheduled-query-pack-sdds')
        )
    if mode == 'coverage':
        test_inputs = dict(
            test_scripts=context.artifact.from_folder('./TA'),
            bullseye_files=context.artifact.from_folder('./build/bullseye'),
            coverage=edr_build / 'coverage',
            coverage_unittest=edr_build / 'coverage/unittest',
            base_sdds=edr_build / 'base/base-sdds',
            edr_sdds=edr_build / 'coverage/SDDS-COMPONENT',
            componenttests=edr_build / 'componenttests',
            qp=unified_artifact(context, 'em.esg', 'develop', 'build/scheduled-query-pack-sdds')
        )

    return test_inputs


@tap.pipeline(version=1, component='sspl-plugin-edr-component')
def edr_plugin(stage: tap.Root, context: tap.PipelineContext, parameters: tap.Parameters):
    mode = parameters.mode or 'release'
    component = tap.Component(name='edr', base_version='1.1.0')

    release_mode = 'release'
    analysis_mode = 'analysis'

    #export TAP_PARAMETER_MODE=release|analysis|coverage*(requires bullseye)
    edr_build = None
    with stage.parallel('build'):
        if mode == release_mode or mode == analysis_mode:
            edr_build = stage.artisan_build(name=release_mode, component=component, image='JenkinsLinuxTemplate5',
                                            mode=release_mode, release_package='./build-files/release-package.xml')
            edr_analysis_build = stage.artisan_build(name=analysis_mode, component=component, image='JenkinsLinuxTemplate5',
                                                     mode=analysis_mode, release_package='./build-files/release-package.xml')

    if mode == 'analysis':
        return

    with stage.parallel('test'):
        machines = (
            ("ubuntu1804",
             tap.Machine('ubuntu1804_x64_server_en_us', inputs=get_inputs(context, edr_build, mode), platform=tap.Platform.Linux)),
            ("centos77", tap.Machine('centos77_x64_server_en_us', inputs=get_inputs(context, edr_build, mode), platform=tap.Platform.Linux)),
            # add other distros here
        )
        coverage_machines = (
            ("centos77", tap.Machine('centos77_x64_server_en_us', inputs=get_inputs(context, edr_build, mode), platform=tap.Platform.Linux)),
        )

        if mode == 'coverage':
            with stage.parallel('combined'):
                for template_name, machine in coverage_machines:
                    stage.task(task_name=template_name, func=combined_task, machine=machine)
        else:
            with stage.parallel('integration'):
                for template_name, machine in machines:
                    stage.task(task_name=template_name, func=robot_task, machine=machine)

            with stage.parallel('component'):
                for template_name, machine in machines:
                    stage.task(task_name=template_name, func=pytest_task, machine=machine)
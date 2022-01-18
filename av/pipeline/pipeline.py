import logging
import os
from typing import Dict

import tap.v1 as tap
from tap._pipeline.tasks import ArtisanInput
from tap._backend import Input


COVFILE_UNITTEST = '/opt/test/inputs/av/sspl-plugin-av-unit.cov'
COVFILE_PYTEST = '/opt/test/inputs/av/sspl-plugin-av-pytest.cov'
COVFILE_ROBOT = '/tmp/sspl-plugin-av-robot.cov' ## Move to /tmp, so that everyone can access it
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
    return branch_name in ('master', "develop") or branch_name.endswith('coverage') or "release" in branch_name

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
                'install', '--upgrade', 'pip', *pip_index_args,
                log_mode=tap.LoggingMode.ON_ERROR)
    machine.run(pip(machine), '--log', '/opt/test/logs/pip.log',
                'install', '-v', *install_args, *pip_index_args,
                log_mode=tap.LoggingMode.ON_ERROR)


def install_requirements(machine: tap.Machine):
    """ install python lib requirements """
    machine.run('bash', machine.inputs.test_scripts / "bin/install_pip_prerequisites.sh")
    pip_install(machine, '-r', machine.inputs.test_scripts / 'requirements.txt')
    try:
        machine.run('groupadd', 'sophos-spl-group')
        machine.run('useradd', 'sophos-spl-user', '-g', 'sophos-spl-group')
    except Exception as ex:
        # the previous command will fail if user already exists. But this is not an error
        print("On adding user and group: {}".format(ex))


def get_suffix():
    global BRANCH_NAME
    if BRANCH_NAME == "develop":
        return ""
    return "-" + BRANCH_NAME


def robot_task_with_env(machine: tap.Machine, test_type=None, environment=None, machine_name=None):
    if machine_name is None:
        machine_name = machine.template
    try:
        # Exclude OSTIA on TAP, since we are testing builds immediately, before they are put in warehouses
        # Exclude MANUAL on TAP
        # Exclude DISABLED on TAP
        # Exclude STRESS on TAP; as some of the tests here will not be appropriate
        robot_exclusion_tags = ['OSTIA', 'MANUAL', 'DISABLED', 'STRESS', 'VQA', 'PRODUCT', 'INTEGRATION']

        if test_type == "product" or test_type == "coverage":
            robot_exclusion_tags.remove('PRODUCT')
            #run VQA only with product tests
            if BRANCH_NAME == "develop" or "vqa" in BRANCH_NAME:
                robot_exclusion_tags.remove('VQA')

        if test_type == "integration" or test_type == "coverage":
            robot_exclusion_tags.remove('INTEGRATION')

        machine.run('bash', machine.inputs.test_scripts / "bin/install_os_packages.sh")
        machine.run(python(machine), machine.inputs.test_scripts / 'RobotFramework.py', *robot_exclusion_tags,
                    environment=environment,
                    timeout=5400)
    finally:
        machine.run(python(machine), machine.inputs.test_scripts / 'move_robot_results.py')
        machine.output_artifact('/opt/test/logs', 'logs')
        machine.output_artifact('/opt/test/results', 'results')
        machine.run('bash', UPLOAD_ROBOT_LOG_SCRIPT, "/opt/test/logs/log.html",
                    "robot" + get_suffix() + "_" + machine_name + "_" + test_type + "-log.html")
        machine.run('bash', UPLOAD_ROBOT_LOG_SCRIPT, "/opt/test/logs/report.html",
                    "robot" + get_suffix() + "_" + machine_name + "_" + test_type + "-report.html")

@tap.timeout(task_timeout=5400)
def robot_task_product(machine: tap.Machine):
    install_requirements(machine)
    robot_task_with_env(machine, "product")

@tap.timeout(task_timeout=5400)
def robot_task_integration(machine: tap.Machine):
    install_requirements(machine)
    robot_task_with_env(machine, "integration")

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


AWS_TIMEOUT = 3 * 3600


@tap.timeout(task_timeout=AWS_TIMEOUT)
def aws_task(machine: tap.Machine):
    try:
        machine.run("bash", machine.inputs.aws_runner / "run_tests_in_aws.sh", timeout=(AWS_TIMEOUT-10))
    finally:
        machine.output_artifact('/opt/test/logs', 'logs')
        machine.run('bash', UPLOAD_ROBOT_LOG_SCRIPT, "/opt/test/logs/log.html",
                    "robot" + get_suffix() + "_aws-log.html")


def unified_artifact(context: tap.PipelineContext, component: str, branch: str, sub_directory: str):
    """Return an input artifact from an unified pipeline build"""
    artifact = context.artifact.from_component(component, branch, org='', storage='esg-build-tested')
    # Using the truediv operator to set the sub directory forgets the storage option
    artifact.sub_directory = sub_directory
    return artifact


def get_inputs(context: tap.PipelineContext, build: ArtisanInput, coverage=False, pipeline=False) -> Dict[str, Input]:
    print(str(build))
    supplement_branch = "released"
    output = 'output'
    # override the av input and get the bullseye coverage build instead
    if coverage:
        output = 'coverage'

    test_inputs = dict(
        test_scripts=context.artifact.from_folder('./TA'),
        bullseye_files=context.artifact.from_folder('./build/bullseye'),  # used for robot upload
        av=build / output,
        # tapartifact upload-file
        # esg-tap-component-store/com.sophos/ssplav-localrep/released/20200219/reputation.zip
        # /mnt/filer6/lrdata/sophos-susi-lrdata/20200219/lrdata/2020021901/reputation.zip
        local_rep=context.artifact.from_component('ssplav-localrep', supplement_branch, None) / 'reputation',
        vdl=context.artifact.from_component('ssplav-vdl', supplement_branch, None) / 'vdl',
        ml_model=context.artifact.from_component('ssplav-mlmodel', supplement_branch, None) / 'model',
    )

    if coverage:
        test_inputs['bazel_tools'] = unified_artifact(context, 'em.esg', 'develop', 'build/bazel-tools')

    if pipeline:
        test_inputs['aws_runner'] = context.artifact.from_folder("./pipeline/aws-runner")

    return test_inputs

@tap.timeout(task_timeout=5400)
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
        pytest_htmldir = os.path.join(coverage_results_dir, 'sspl-plugin-av-pytest')
        machine.run('bash', UPLOAD_SCRIPT,
                    environment={
                        'COVFILE': COVFILE_PYTEST,
                        'COVSRCDIR': COVSRCDIR,
                        'COV_HTML_BASE': 'sspl-plugin-av-pytest' + suffix,
                        'htmldir': pytest_htmldir,
                    })
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
            robot_task_with_env(machine,
                                test_type="coverage",
                                environment={
                                    'COVFILE': COVFILE_ROBOT,
                                    'COVSRCDIR': COVSRCDIR,
                                },
                                machine_name="coverage")
        except tap.exceptions.PipelineProcessExitNonZeroError as e:
            exception = e

        machine.run('rm', '-f', '/tmp/BullseyeCoverageEnv.txt')
        robot_htmldir = os.path.join(coverage_results_dir, 'sspl-plugin-av-robot')
        machine.run('bash', UPLOAD_SCRIPT,
                    environment={
                        'COVFILE': COVFILE_ROBOT,
                        'COVSRCDIR': COVSRCDIR,
                        'COV_HTML_BASE': 'sspl-plugin-av-robot' + suffix,
                        'htmldir': robot_htmldir,
                    })
        machine.run('cp', COVFILE_ROBOT, coverage_results_dir)

        # generate combined coverage html results and upload to allegro
        machine.run('covmerge', '--create', '--file', COVFILE_COMBINED,
                    COVFILE_UNITTEST,
                    COVFILE_PYTEST,
                    COVFILE_ROBOT
                    )
        combined_htmldir = os.path.join(coverage_results_dir, 'sspl-plugin-av-combined')
        machine.run('bash', UPLOAD_SCRIPT,
                    environment={
                        'COVFILE': COVFILE_COMBINED,
                        'COVSRCDIR': COVSRCDIR,
                        'COV_HTML_BASE': 'sspl-plugin-av-combined' + suffix,
                        'htmldir': combined_htmldir,
                    })

        # publish combined html results and coverage file to artifactory
        machine.run('cp', COVFILE_COMBINED, coverage_results_dir)

        COVERAGE_NORMALISE_JSON = os.path.join(coverage_results_dir, "test_coverage.json")
        COVERAGE_MIN_FUNCTION = 70
        COVERAGE_MIN_CONDITION = 70
        test_coverage_script = machine.inputs.bazel_tools / 'tools' / 'src' / 'bullseye' / 'test_coverage.py'
        test_coverage_args = [
            'python', '-u', str(test_coverage_script),
            COVFILE_COMBINED, '--output', COVERAGE_NORMALISE_JSON,
            '--min-function', str(COVERAGE_MIN_FUNCTION),
            '--min-condition', str(COVERAGE_MIN_CONDITION)
        ]
        global BRANCH_NAME
        if BRANCH_NAME == 'develop':
            test_coverage_args += ['--upload', '--upload-job', 'UnifiedPipelines/linuxep/sspl-plugin-anti-virus']
        machine.run(*test_coverage_args)

        if exception is not None:
            raise exception
    finally:
        machine.output_artifact('/opt/test/results', 'results')
        machine.output_artifact('/opt/test/logs', 'logs')
        machine.output_artifact(coverage_results_dir, 'coverage')

def get_test_machines(test_inputs, parameters: tap.Parameters):
    test_environments = {'ubuntu1804': 'ubuntu1804_x64_server_en_us',
                         'ubuntu2004': 'ubuntu2004_x64_server_en_us',
                         'centos77': 'centos77_x64_server_en_us',
                         }

    if parameters.run_centos8_tap != 'false':
        test_environments['centos8'] = 'centos82_x64_server_en_us'

    ret = []
    for name, image in test_environments.items():
        ret.append((
            name,
            tap.Machine(image, inputs=test_inputs, platform=tap.Platform.Linux)
        ))
    return ret

def decide_whether_to_run_aws_tests(parameters: tap.Parameters, context: tap.PipelineContext) -> bool:
    if parameters.force_run_aws_tests != 'false':
        return True
    if parameters.inhibit_run_aws_tests != 'false':
        return False
    branch = context.branch
    return branch in ('master', "develop") or branch.startswith("release/")


def decide_whether_to_do_coverage(parameters: tap.Parameters, context: tap.PipelineContext) -> bool:
    if parameters.force_run_coverage != 'false':
        return True
    if parameters.inhibit_run_coverage != 'false':
        return False
    branch = context.branch
    return has_coverage_build(branch)


def decide_whether_to_run_cppcheck(parameters: tap.Parameters, context: tap.PipelineContext) -> bool:
    if parameters.force_run_cppcheck != 'false':
        return True
    if parameters.inhibit_run_cppcheck != 'false':
        return False
    branch = context.branch
    return not branch.startswith("release/")


def get_base_version():
    import xml.dom.minidom
    package_path = "./build-files/release-package.xml"
    if os.path.isfile(package_path):
        dom = xml.dom.minidom.parseString(open(package_path).read())
        package = dom.getElementsByTagName("package")[0]
        version = package.getAttribute("version")
        if version:
            logger.info("Extracted version from release-package.xml: %s", version)
            return version

    logger.info("CWD: %s", os.getcwd())
    logger.info("DIR CWD: %s", str(os.listdir(os.getcwd())))

    version = "1.0.6"
    logger.info("Using default version: %s", version)
    return version


@tap.pipeline(component='sspl-plugin-anti-virus', root_sequential=False)
def av_plugin(stage: tap.Root, context: tap.PipelineContext, parameters: tap.Parameters):
    global BRANCH_NAME
    BRANCH_NAME = context.branch

    run_tests = parameters.run_tests != 'false'
    run_aws_tests = decide_whether_to_run_aws_tests(parameters, context)
    do_coverage: bool = decide_whether_to_do_coverage(parameters, context)
    do_cppcheck: bool = decide_whether_to_run_cppcheck(parameters, context)

    coverage_build = context.artifact.build()

    # section include to allow classic build to continue to work. To run unified pipeline local because of this check
    # export TAP_PARAMETER_MODE=release|analysis|coverage*(requires bullseye)
    if parameters.mode:
        component = tap.Component(name='sspl-plugin-anti-virus', base_version=get_base_version())
        build_image = 'JenkinsLinuxTemplate6'
        release_package = "./build-files/release-package.xml"
        with stage.parallel('build'):
            if do_cppcheck:
                av_cpp_check = stage.artisan_build(name="cpp-check", component=component, image=build_image,
                                                   mode="cppcheck", release_package=release_package)

            nine_nine_nine_mode = '999'
            nine_nine_nine_build = stage.artisan_build(name=nine_nine_nine_mode, component=component, image=build_image,
                                                       mode=nine_nine_nine_mode, release_package=release_package)

            av_build = stage.artisan_build(name="normal_build", component=component, image=build_image,
                                           mode=parameters.mode, release_package=release_package)

            if do_coverage:
                coverage_build = stage.artisan_build(name="coverage_build", component=component, image=build_image,
                                               mode="coverage", release_package=release_package)
    else:
        # Non-unified build
        av_build = context.artifact.build()

    if run_tests:
        with stage.parallel('testing'):
            test_inputs = get_inputs(context, av_build)
            with stage.parallel('TA'):
                for (name, machine) in get_test_machines(test_inputs, parameters):
                    stage.task(task_name=name+"_component", func=pytest_task, machine=machine)

                for (name, machine) in get_test_machines(test_inputs, parameters):
                    stage.task(task_name=name+"_product", func=robot_task_product, machine=machine)

                for (name, machine) in get_test_machines(test_inputs, parameters):
                    stage.task(task_name=name+"_integration", func=robot_task_integration, machine=machine)

            if do_coverage:
                with stage.parallel('coverage'):
                    coverage_inputs = get_inputs(context, coverage_build, coverage=True)
                    machine_bullseye_test = tap.Machine('ubuntu1804_x64_server_en_us',
                                                        inputs=coverage_inputs,
                                                        platform=tap.Platform.Linux)
                    stage.task(task_name='ubuntu1804_x64_combined', func=bullseye_coverage_task, machine=machine_bullseye_test)

    if run_aws_tests:
        test_inputs = get_inputs(context, av_build, pipeline=True)
        machine = tap.Machine('ubuntu1804_x64_server_en_us', inputs=test_inputs, platform=tap.Platform.Linux)
        stage.task("aws_tests", func=aws_task, machine=machine)

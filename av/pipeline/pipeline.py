import logging
import os
from typing import Dict

import tap.v1 as tap
from tap._pipeline.tasks import ArtisanInput
from tap._backend import Input, TaskOutput

# TAP Pipeline API : https://docs.sophos-ops.com/pipeline.html

INPUTS_DIR = '/opt/test/inputs'
LOGS_DIR = '/opt/test/logs'
RESULTS_DIR = '/opt/test/results'
COVERAGE_DIR = '/opt/test/coverage'
COVSRCDIR = os.path.join(INPUTS_DIR, 'av', 'src')
COVFILE_BASE = os.path.join(INPUTS_DIR, 'av', 'sspl-plugin-av.cov')
COVFILE_UNITTEST = os.path.join(INPUTS_DIR, 'av', 'sspl-plugin-av-unit.cov')
BULLSEYE_SCRIPT_DIR = os.path.join(INPUTS_DIR, 'bullseye_files')
UPLOAD_SCRIPT = os.path.join(BULLSEYE_SCRIPT_DIR, 'uploadResults.sh')
UPLOAD_ROBOT_LOG_SCRIPT = os.path.join(BULLSEYE_SCRIPT_DIR, 'uploadRobotLog.sh')

logger = logging.getLogger(__name__)

BRANCH_NAME = "master"


def is_debian_based(machine: tap.Machine):
    return machine.template.startswith("ubuntu")


def is_redhat_based(machine: tap.Machine):
    return machine.template.startswith("centos")


def include_cifs_for_machine_name(name: str, template):
    return True


def include_ntfs_for_machine_name(name: str, template):
    no_ntfs = (
        'amazonlinux2023',
        'oracle7',
        'oracle8',
        'rhel7',
        'rhel8',
        'rhel9',
        'sles12',
        'sles15'
    )
    if name in no_ntfs:
        return False
    return True


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
    # machine.run('bash', machine.inputs.test_scripts / "bin/install_pip_prerequisites.sh")
    try:
        machine.run("which", python(machine), return_exit_code=True)
        machine.run("which", pip(machine), return_exit_code=True)
        machine.run(python(machine), "-V", return_exit_code=True)
        machine.run("which", "python", return_exit_code=True)
        machine.run("python", "-V", return_exit_code=True)
    except Exception:
        pass

    pip_install(machine, '-r', machine.inputs.test_scripts / 'requirements.txt')


def get_suffix():
    global BRANCH_NAME
    if BRANCH_NAME == "develop":
        return ""
    return "-" + BRANCH_NAME


def robot_task_with_env(machine: tap.Machine, include_tag: str, robot_args: str = None, environment=None,
                        machine_name: str=None):
    if machine_name is None:
        machine_name = machine.template

    assert include_tag or robot_args

    try:
        # Exclude OSTIA on TAP, since we are testing builds immediately, before they are put in warehouses
        # Exclude MANUAL on TAP
        # Exclude DISABLED on TAP
        # Exclude STRESS on TAP; as some tests here will not be appropriate
        robot_exclusion_tags = ['OSTIA', 'MANUAL', 'DISABLED', 'STRESS']
        if machine_name.startswith('centos9stream'):
            #  As of 2023-06-15 CentOS 9 Stream doesn't support NFSv2
            robot_exclusion_tags.append("nfsv2")

        if machine_name.startswith('ubuntu2204'):
            # On 2023-08-18 NFSv2 tests failed on Ubuntu 22.04 as NFSv2 was no longer supported (?)
            robot_exclusion_tags.append("nfsv2")

        cifs = getattr(machine, "cifs_supported", 1)
        ntfs = getattr(machine, "ntfs_supported", 1)
        sspl_name = getattr(machine, "sspl_name", "unknown")
        install_command = ['bash', machine.inputs.test_scripts / "bin/install_os_packages.sh"]

        if cifs and include_cifs_for_machine_name(machine_name, machine.template):
            print("CIFS enabled:", machine_name, sspl_name, cifs, id(machine))
        else:
            print("CIFS disabled:", machine_name, sspl_name, cifs, id(machine))
            robot_exclusion_tags.append("cifs")
            install_command.append("--without-cifs")

        if ntfs and include_ntfs_for_machine_name(machine_name, machine.template):
            print("NTFS enabled:", machine_name, sspl_name, ntfs, id(machine))
        else:
            print("NTFS disabled:", machine_name, sspl_name, ntfs, id(machine))
            robot_exclusion_tags.append("ntfs")
            install_command.append("--without-ntfs")

        machine.run(*install_command)

        machine.run('mkdir', '-p', '/opt/test/coredumps')

        if include_tag:
            include, *exclude = include_tag.split("NOT")
            include = include.split("AND")
            robot_exclusion_tags.extend(exclude)

            machine.run(python(machine), machine.inputs.test_scripts / 'RobotFramework.py',
                        '--include', *include,
                        '--exclude', *robot_exclusion_tags,
                        environment=environment,
                        timeout=5400)
        elif robot_args:
            machine.run(robot_args, 'python3', machine.inputs.test_scripts / 'RobotFramework.py',
                        '--exclude', *robot_exclusion_tags,
                        environment=environment, timeout=5400)

    finally:
        machine.run(python(machine), machine.inputs.test_scripts / 'move_robot_results.py')
        machine.output_artifact('/opt/test/logs', 'logs')
        machine.output_artifact('/opt/test/results', 'results')
        machine.output_artifact('/opt/test/coredumps', 'coredumps', raise_on_failure=False)
        if include_tag:
            machine.run('bash', UPLOAD_ROBOT_LOG_SCRIPT, "/opt/test/results/log.html",
                        "av" + get_suffix() + "_" + machine_name + "_" + include_tag + "-log.html")
            machine.run('bash', UPLOAD_ROBOT_LOG_SCRIPT, "/opt/test/results/report.html",
                        "av" + get_suffix() + "_" + machine_name + "_" + include_tag + "-report.html")
        elif robot_args:
            machine.run('bash', UPLOAD_ROBOT_LOG_SCRIPT, "/opt/test/results/log.html",
                        "av" + get_suffix() + "_" + machine_name + "_" + robot_args + "-log.html")
            machine.run('bash', UPLOAD_ROBOT_LOG_SCRIPT, "/opt/test/results/report.html",
                        "av" + get_suffix() + "_" + machine_name + "_" + robot_args + "-report.html")


@tap.timeout(task_timeout=120)
def robot_task(machine: tap.Machine, include_tag: str, robot_args: str, machine_name: str = None):
    print("robot_task for", machine_name, id(machine))
    install_requirements(machine)
    robot_task_with_env(machine, include_tag, robot_args, machine_name=machine_name)


def pytest_task_with_env(machine: tap.Machine, environment=None):
    try:
        tests_dir = str(machine.inputs.test_scripts)
        args = [python(machine), '-u', '-m', 'pytest', tests_dir,
                '-o', 'log_cli=true',
                '--html=/opt/test/results/report.html'
                ]
        machine.run(*args, environment=environment)
        machine.run('ls', '-l', '/opt/test/results')
        machine.run('ls', '-l', '/opt/test/logs')
    finally:
        machine.output_artifact('/opt/test/results', 'results')
        machine.output_artifact('/opt/test/logs', 'logs')


def pytest_task(machine: tap.Machine):
    install_requirements(machine)
    pytest_task_with_env(machine)


def unified_artifact(context: tap.PipelineContext, component: str, branch: str, sub_directory: str):
    """Return an input artifact from a unified pipeline build"""
    artifact = context.artifact.from_component(component, branch, org='', storage='esg-build-tested')
    # Using the truediv operator to set the subdirectory forgets the storage option
    artifact.sub_directory = sub_directory
    return artifact


def get_inputs(context: tap.PipelineContext, build: ArtisanInput, coverage=False) -> Dict[str, Input]:
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
        ml_model=context.artifact.from_component('ssplav-mlmodel3-x86_64', supplement_branch, None) / 'model',
        dataseta=context.artifact.from_component('ssplav-dataseta', supplement_branch, None) / 'dataseta',
    )
    test_inputs['sdds3_utils'] = unified_artifact(context, 'winep.sau', 'develop', 'build/Linux-x64/SDDS3-Utils')

    if coverage:
        test_inputs['bazel_tools'] = unified_artifact(context, 'em.esg', 'develop', 'build/bazel-tools')
        test_inputs['tap_test_output_from_build'] = build / "coverage_tap_test_output"
    else:
        test_inputs['tap_test_output_from_build'] = build / "tap_test_output"

    return test_inputs


def bullseye_coverage_pytest_task(machine: tap.Machine):
    install_requirements(machine)

    machine.run('rm', '-rf', COVERAGE_DIR)
    machine.run('mkdir', '-p', COVERAGE_DIR)
    covfile = os.path.join(COVERAGE_DIR, "sspl-plugin-av-pytest.cov")

    machine.run('cp', COVFILE_BASE, covfile)

    pytest_task_with_env(machine, environment={
        'COVFILE': covfile,
        'COVSRCDIR': COVSRCDIR,
    })

    machine.output_artifact(COVERAGE_DIR, output=machine.outputs.covfile)


@tap.timeout(task_timeout=120)
def bullseye_coverage_robot_task(machine: tap.Machine, include_tag: str):
    install_requirements(machine)

    machine.run('rm', '-rf', COVERAGE_DIR)
    machine.run('mkdir', '-p', COVERAGE_DIR)

    # use /tmp so that it's accessible to all users/processes
    covfile = os.path.join("/tmp", "sspl-plugin-av-robot-" + include_tag + ".cov")
    machine.run('cp', COVFILE_BASE, covfile)
    machine.run('chmod', '666', covfile)

    # set bullseye environment in a file, so that daemons pick up the settings too
    machine.run('bash', os.path.join(BULLSEYE_SCRIPT_DIR, "createBullseyeCoverageEnv.sh"),
                covfile, COVSRCDIR)

    exception = None
    try:
        robot_task_with_env(machine,
                            include_tag=include_tag,
                            environment={
                                'COVFILE': covfile,
                                'COVSRCDIR': COVSRCDIR,
                            },
                            machine_name="coverage")
    except tap.exceptions.PipelineProcessExitNonZeroError as e:
        exception = e

    machine.run('rm', '-f', '/tmp/BullseyeCoverageEnv.txt')

    machine.run('cp', covfile, COVERAGE_DIR)
    machine.output_artifact(COVERAGE_DIR, output=machine.outputs.covfile)

    if exception is not None:
        raise exception


def bullseye_upload(machine: tap.Machine, name, covfile=None):
    suffix = get_suffix()
    coverage_results_dir = os.path.join(RESULTS_DIR, 'coverage')

    if covfile is None:
        covfile = os.path.join(coverage_results_dir, name + ".cov")

    htmldir = os.path.join(coverage_results_dir, name)
    machine.run('bash', UPLOAD_SCRIPT,
                environment={
                    'COVFILE': covfile,
                    'COVSRCDIR': COVSRCDIR,
                    'COV_HTML_BASE': name + suffix,
                    'htmldir': htmldir,
                })

    return covfile


def bullseye_merge(machine: tap.Machine, output_name, *input_names):
    coverage_results_dir = os.path.join(RESULTS_DIR, 'coverage')

    output_file = os.path.join(coverage_results_dir, output_name + ".cov")
    input_files = {os.path.join(coverage_results_dir, input_name + ".cov") for input_name in input_names}

    machine.run('covmerge', '--create', '--file', output_file, *input_files)

    return output_file


def bullseye_test_coverage(machine: tap.Machine, covfile):
    coverage_results_dir = os.path.join(RESULTS_DIR, 'coverage')

    COVERAGE_NORMALISE_JSON = os.path.join(coverage_results_dir, "test_coverage.json")
    COVERAGE_MIN_FUNCTION = 70
    COVERAGE_MIN_CONDITION = 70
    test_coverage_script = machine.inputs.bazel_tools / 'tools' / 'src' / 'bullseye' / 'test_coverage.py'
    test_coverage_args = [
        'python', '-u', str(test_coverage_script),
        covfile, '--output', COVERAGE_NORMALISE_JSON,
        '--min-function', str(COVERAGE_MIN_FUNCTION),
        '--min-condition', str(COVERAGE_MIN_CONDITION)
    ]
    global BRANCH_NAME
    if BRANCH_NAME == 'develop':
        test_coverage_args += ['--upload', '--upload-job', 'UnifiedPipelines/linuxep/sspl-plugin-anti-virus']
    machine.run(*test_coverage_args)


def bullseye_coverage_combine_task(machine: tap.Machine, include_tag: str):
    install_requirements(machine)

    # str(machine.inputs.covfiles) = /opt/test/inputs/covfiles
    # /opt/test/inputs/covfiles/av_plugin.testing.coverage.ubuntu1804_bullseye_component/machine/sspl-plugin-av-pytest.cov
    machine.run('ls', '-Rl', machine.inputs.covfiles)

    coverage_results_dir = os.path.join(RESULTS_DIR, 'coverage')
    machine.run('rm', '-rf', coverage_results_dir)
    machine.run('mkdir', coverage_results_dir)

    try:
        # copy all the covfiles to coverage_results_dir
        machine.run('cp', COVFILE_UNITTEST, coverage_results_dir)
        machine.run('find', machine.inputs.covfiles, '-type', 'f', '-exec', 'cp', r'\{\}', coverage_results_dir, r'\;')

        machine.run('ls', '-Rl', coverage_results_dir)

        # individual coverage
        bullseye_upload(machine, 'sspl-plugin-av-unit')
        bullseye_upload(machine, 'sspl-plugin-av-pytest')
        robot_covfiles = []
        for include in include_tag.split():
            covfile = 'sspl-plugin-av-robot-' + include
            bullseye_upload(machine, covfile)
            robot_covfiles.append(covfile)

        # robot coverage
        bullseye_merge(machine,
                       'sspl-plugin-av-robot',
                       *robot_covfiles)
        bullseye_upload(machine, 'sspl-plugin-av-robot')

        # combined coverage
        bullseye_merge(machine,
                       'sspl-plugin-av-combined',
                       'sspl-plugin-av-unit', 'sspl-plugin-av-pytest', 'sspl-plugin-av-robot')
        covfile_combined = bullseye_upload(machine, 'sspl-plugin-av-combined')

        # check (and report) overall coverage
        bullseye_test_coverage(machine, covfile_combined)
    finally:
        machine.output_artifact(coverage_results_dir, 'coverage')


def get_test_machines(test_inputs, parameters: tap.Parameters):
    if parameters.run_tests == "false":
        return []

    test_environments = {}

    if parameters.run_amazon_2 != "false":
        test_environments['amazonlinux2'] = 'amzlinux2_x64_server_en_us'

    if parameters.run_amazon_2023 != "false":
        test_environments['amazonlinux2023'] = 'amzlinux2023_x64_server_en_us'

    if parameters.run_centos_7 != "false":
        test_environments['centos79'] = 'centos7_x64_aws_server_en_us'

    if parameters.run_centos_stream_8 != "false":
        test_environments['centos8stream'] = 'centos8stream_x64_aws_server_en_us'

    if parameters.run_centos_stream_9 != "false":
        test_environments['centos9stream'] = 'centos9stream_x64_aws_server_en_us'

    if parameters.run_debian_10 != "false":
        test_environments['debian10'] = 'debian10_x64_aws_server_en_us'

    if parameters.run_debian_11 != "false":
        test_environments['debian11'] = 'debian11_x64_aws_server_en_us'

    if parameters.run_debian_12 != "false":
        test_environments['debian12'] = 'debian12_x64_aws_server_en_us'

    if parameters.run_oracle_7 != "false":
        test_environments['oracle7'] = 'oracle79_x64_aws_server_en_us'

    if parameters.run_oracle_8 != "false":
        test_environments['oracle8'] = 'oracle87_x64_aws_server_en_us'

    if parameters.run_rhel_7 != "false":
        test_environments['rhel7'] = 'rhel79_x64_aws_server_en_us'

    if parameters.run_rhel_8 != "false":
        test_environments['rhel8'] = 'rhel87_x64_aws_server_en_us'

    if parameters.run_rhel_9 != "false":
        test_environments['rhel9'] = 'rhel91_x64_aws_server_en_us'

    if parameters.run_sles_12 != "false":
        test_environments['sles12'] = 'sles12_x64_sp5_aws_server_en_us'

    if parameters.run_sles_15 != "false":
        test_environments['sles15'] = 'sles15_x64_sp4_aws_server_en_us'

    if parameters.run_ubuntu_18_04 != "false":
        test_environments['ubuntu1804'] = 'ubuntu1804_x64_server_en_us'

    if parameters.run_ubuntu_20_04 != "false":
        test_environments['ubuntu2004'] = 'ubuntu2004_x64_server_en_us'

    if parameters.run_ubuntu_22_04 != "false":
        test_environments['ubuntu2204'] = 'ubuntu2204_x64_aws_server_en_us'

    ret = []
    for name, image in test_environments.items():
        machine = tap.Machine(image, inputs=test_inputs, platform=tap.Platform.Linux)
        machine.cifs_supported = include_cifs_for_machine_name(name, image)
        machine.ntfs_supported = include_ntfs_for_machine_name(name, image)
        machine.sspl_name = name
        ret.append((name, machine))
    return ret


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

    raise Exception("Failed to extract version from release-package.xml")


@tap.pipeline(root_sequential=False)
def av_plugin(stage: tap.Root, context: tap.PipelineContext, parameters: tap.Parameters):
    global BRANCH_NAME
    BRANCH_NAME = context.branch

    run_tests: bool = parameters.run_tests != 'false'

    # Add args to pass env vars to RobotFramework.py call in test runs
    robot_args_list = []
    if parameters.test:
        robot_args_list.append("TEST=" + parameters.test)
    elif parameters.suite:
        robot_args_list.append("SUITE=" + parameters.suite)
    robot_args = " ".join(robot_args_list)
    # robot_task_with_env will parse this string
    include_tag = parameters.include_tag or "productNOTav_basicNOTavscanner av_basicORavscanner " \
                                            "integrationNOTavbaseNOTav_health avbase av_health"

    do_coverage: bool = parameters.mode == 'coverage'
    do_cppcheck: bool = decide_whether_to_run_cppcheck(parameters, context)
    do_999_build: bool = parameters.do_999_build != 'false'

    component = tap.Component(name='sspl-plugin-anti-virus', base_version=get_base_version())
    build_image = 'centos79_x64_build_20230202'
    release_package = "./build-files/release-package.xml"
    with stage.parallel('build'):
        if do_cppcheck:
            av_cpp_check = stage.artisan_build(name="cpp-check", component=component, image=build_image,
                                               mode="cppcheck", release_package=release_package)

        if do_999_build:
            nine_nine_nine_mode = '999'
            nine_nine_nine_build = stage.artisan_build(name=nine_nine_nine_mode, component=component, image=build_image,
                                                       mode=nine_nine_nine_mode, release_package=release_package)

        av_build = stage.artisan_build(name="normal_build", component=component, image=build_image,
                                       mode="release", release_package=release_package)

        if do_coverage:
            coverage_build = stage.artisan_build(name="coverage_build", component=component, image=build_image,
                                                 mode="coverage", release_package=release_package)

    with stage.parallel('testing'):
        # Coverage next, since that is the next slowest
        if do_coverage:
            with stage.parallel('coverage'):
                coverage_inputs = get_inputs(context, coverage_build, coverage=True)

                with stage.parallel('pytest'):
                    machine_bullseye_pytest = \
                        tap.Machine('ubuntu1804_x64_server_en_us',
                                    inputs=coverage_inputs,
                                    outputs={'covfile': TaskOutput('covfiles')},
                                    platform=tap.Platform.Linux)
                    stage.task(task_name='component',
                               func=bullseye_coverage_pytest_task,
                               machine=machine_bullseye_pytest)

                with stage.parallel('robot'):
                    for include in include_tag.split():
                        machine_bullseye_robot = \
                            tap.Machine('ubuntu1804_x64_server_en_us',
                                        inputs=coverage_inputs,
                                        outputs={'covfile': TaskOutput('covfiles')},
                                        platform=tap.Platform.Linux)
                        stage.task(task_name=include,
                                   func=bullseye_coverage_robot_task,
                                   machine=machine_bullseye_robot,
                                   include_tag=include)

                combine_inputs = coverage_inputs
                combine_inputs['covfiles'] = TaskOutput('covfiles')
                machine_bullseye_combine = \
                    tap.Machine('ubuntu1804_x64_server_en_us',
                                inputs=combine_inputs,
                                platform=tap.Platform.Linux)
                stage.task(task_name='combine',
                           func=bullseye_coverage_combine_task,
                           machine=machine_bullseye_combine,
                           include_tag=include_tag)

        if run_tests:
            with stage.parallel('TA'):
                test_inputs = get_inputs(context, av_build)
                # Robot before pytest, since pytest is quick
                with stage.parallel('robot'):
                    if robot_args:
                        with stage.parallel("integration"):
                            for (name, machine) in get_test_machines(test_inputs, parameters):
                                stage.task(task_name=name, func=robot_task, machine=machine,
                                           robot_args=robot_args, include_tag="", machine_name=name)
                    else:
                        for include in include_tag.split():
                            with stage.parallel(include):
                                for (name, machine) in get_test_machines(test_inputs, parameters):
                                    stage.task(task_name=name, func=robot_task, machine=machine,
                                               include_tag=include, robot_args="", machine_name=name)

                with stage.parallel('pytest'):
                    with stage.parallel('component'):
                        for (name, machine) in get_test_machines(test_inputs, parameters):
                            stage.task(task_name=name, func=pytest_task, machine=machine)

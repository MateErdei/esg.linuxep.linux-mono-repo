# Copyright 2023 Sophos Limited. All rights reserved.
import os
from typing import Dict

import tap.v1 as tap
from tap._backend import Input, TaskOutput
from tap._pipeline.tasks import ArtisanInput

from pipeline.common import unified_artifact, get_test_machines, pip_install, get_suffix, get_robot_args, \
    python, ROBOT_TEST_TIMEOUT, TASK_TIMEOUT
from pipeline import common


INPUTS_DIR = '/opt/test/inputs'
RESULTS_DIR = '/opt/test/results'
COVERAGE_DIR = '/opt/test/coverage'
COVSRCDIR = os.path.join(INPUTS_DIR, 'av', 'src')
COVFILE_BASE = os.path.join(INPUTS_DIR, 'av', 'sspl-plugin-av.cov')
COVFILE_UNITTEST = os.path.join(INPUTS_DIR, 'av', 'sspl-plugin-av-unit.cov')
BULLSEYE_SCRIPT_DIR = os.path.join(INPUTS_DIR, 'bullseye_files')
UPLOAD_SCRIPT = os.path.join(BULLSEYE_SCRIPT_DIR, 'uploadResults.sh')
UPLOAD_ROBOT_LOG_SCRIPT = os.path.join(BULLSEYE_SCRIPT_DIR, 'uploadRobotLog.sh')


def include_cifs_for_machine_name(name: str, template):
    no_samba = (
        'ubuntu2204'
    )
    distro = name.split("_")[1]
    return distro not in no_samba


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
    distro = name.split("_")[1]
    if distro in no_ntfs:
        return False
    return True


def pip(machine: tap.Machine):
    return "pip3"

def install_requirements(machine: tap.Machine):
    """ install python lib requirements """
    try:
        machine.run("which", python(machine), return_exit_code=True, timeout=5)
        machine.run("which", pip(machine), return_exit_code=True, timeout=5)
        machine.run(python(machine), "-V", return_exit_code=True, timeout=5)
        machine.run("which", "python", return_exit_code=True, timeout=5)
        machine.run("python", "-V", return_exit_code=True, timeout=5)
    except Exception:
        pass

    pip_install(machine, '-r', machine.inputs.test_scripts / 'requirements.txt')


def robot_task_with_env(machine: tap.Machine, include_tag: str, branch_name: str, robot_args: str = None,
                        environment=None,
                        machine_name: str = None):
    if machine_name is None:
        machine_name = machine.template

    arch, platform = machine_name.split("_")

    assert include_tag or robot_args

    try:
        # Exclude OSTIA on TAP, since we are testing builds immediately, before they are put in warehouses
        # Exclude MANUAL on TAP
        # Exclude DISABLED on TAP
        # Exclude STRESS on TAP; as some tests here will not be appropriate
        robot_exclusion_tags = ['OSTIA', 'MANUAL', 'DISABLED', 'STRESS',
                                f"exclude_{platform.lower()}", f"exclude_{arch.lower()}",
                                f"exclude_{machine_name.lower()}"]
        if platform in ('centos9stream', 'ubuntu2204'):
            #  As of 2023-06-15 CentOS 9 Stream doesn't support NFSv2
            #  As of 2023-10-27 Ubuntu 22.04 doesn't support NFSv2
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

        machine.run(*install_command, timeout=600)

        machine.run('mkdir', '-p', '/opt/test/coredumps', timeout=5)

        if include_tag:
            machine.run(python(machine), machine.inputs.test_scripts / 'RobotFramework.py',
                        '--include', include_tag,
                        '--exclude', *robot_exclusion_tags,
                        environment=environment,
                        timeout=ROBOT_TEST_TIMEOUT)
        elif robot_args:
            machine.run(robot_args, 'python3', machine.inputs.test_scripts / 'RobotFramework.py',
                        '--exclude', *robot_exclusion_tags,
                        environment=environment, timeout=ROBOT_TEST_TIMEOUT)

    finally:
        machine.run(python(machine), machine.inputs.test_scripts / 'move_robot_results.py', timeout=60)
        machine.output_artifact('/opt/test/logs', 'logs')
        machine.output_artifact('/opt/test/results', 'results')
        machine.output_artifact('/opt/test/coredumps', 'coredumps', raise_on_failure=False)
        if include_tag:
            machine.run('bash', UPLOAD_ROBOT_LOG_SCRIPT, "/opt/test/results/log.html",
                        "av" + get_suffix(branch_name) + "_" + machine_name + "_" + include_tag + "-log.html",
                        timeout=120)
            machine.run('bash', UPLOAD_ROBOT_LOG_SCRIPT, "/opt/test/results/report.html",
                        "av" + get_suffix(branch_name) + "_" + machine_name + "_" + include_tag + "-report.html",
                        timeout=120)
        elif robot_args:
            machine.run('bash', UPLOAD_ROBOT_LOG_SCRIPT, "/opt/test/results/log.html",
                        "av" + get_suffix(branch_name) + "_" + machine_name + "_" + robot_args + "-log.html",
                        timeout=120)
            machine.run('bash', UPLOAD_ROBOT_LOG_SCRIPT, "/opt/test/results/report.html",
                        "av" + get_suffix(branch_name) + "_" + machine_name + "_" + robot_args + "-report.html",
                        timeout=120)


@tap.timeout(task_timeout=TASK_TIMEOUT)
def robot_task(machine: tap.Machine, include_tag: str, branch_name: str, robot_args: str, machine_name: str = None):
    print("robot_task for", machine_name, id(machine))
    install_requirements(machine)
    robot_task_with_env(machine, include_tag, branch_name, robot_args, machine_name=machine_name)


def pytest_task_with_env(machine: tap.Machine, environment=None):
    try:
        tests_dir = str(machine.inputs.test_scripts)
        args = [python(machine), '-u', '-m', 'pytest', tests_dir,
                '-o', 'log_cli=true',
                '--html=/opt/test/results/report.html'
                ]
        machine.run(*args, environment=environment, timeout=3600)
        machine.run('ls', '-l', '/opt/test/results', timeout=5)
        machine.run('ls', '-l', '/opt/test/logs', timeout=5)
    finally:
        machine.output_artifact('/opt/test/results', 'results')
        machine.output_artifact('/opt/test/logs', 'logs')


def pytest_task(machine: tap.Machine):
    install_requirements(machine)
    pytest_task_with_env(machine)


def get_inputs(context: tap.PipelineContext,
               build: ArtisanInput,
               coverage: bool = False,
               bazel: bool = True,
               arch: str = "x86_64") -> Dict[str, Input]:
    if build is None:
        return None

    supplement_branch = "released"

    test_inputs = dict(
        test_scripts=context.artifact.from_folder('./av/TA'),
        bullseye_files=context.artifact.from_folder('./av/build/bullseye'),  # used for robot upload
        local_rep=context.artifact.from_component('ssplav-localrep', supplement_branch, None) / 'reputation',
        ml_model=context.artifact.from_component('ssplav-mlmodel3-'+arch, supplement_branch, None) / 'model',
        dataseta=context.artifact.from_component('ssplav-dataseta', supplement_branch, None) / 'dataseta',
    )
    test_inputs['sdds3_utils'] = unified_artifact(context, 'winep.sau', 'develop', 'build/Linux-x64/SDDS3-Utils')

    if coverage:
        # override the av input and get the bullseye coverage build instead
        test_inputs['bazel_tools'] = unified_artifact(context, 'em.esg', 'develop', 'build/bazel-tools')
        test_inputs['tap_test_output_from_build'] = build / "coverage_tap_test_output"
        test_inputs["av/SDDS-COMPONENT"] = build / 'coverage/SDDS-COMPONENT'
        test_inputs["av/base-sdds"] = build / 'coverage/base_sdds'
    elif bazel:
        if arch == "x86_64":
            build_arch = "x64"
        elif arch == "arm64":
            build_arch = "arm64"

        test_inputs['tap_test_output'] = build / f"av/linux_{build_arch}_rel/tap_test_output"
        test_inputs["av/SDDS-COMPONENT"] = build / f'av/linux_{build_arch}_rel/installer'
        test_inputs["av/base-sdds"] = build / f'base/linux_{build_arch}_rel/installer'

    else:
        test_inputs['tap_test_output_from_build'] = build / "tap_test_output"
        test_inputs["av/SDDS-COMPONENT"] = build / 'output/SDDS-COMPONENT'
        test_inputs["av/base-sdds"] = build / 'output/base_sdds'

    return test_inputs


def bullseye_coverage_pytest_task(machine: tap.Machine):
    install_requirements(machine)

    machine.run('rm', '-rf', COVERAGE_DIR, timeout=60)
    machine.run('mkdir', '-p', COVERAGE_DIR, timeout=5)
    covfile = os.path.join(COVERAGE_DIR, "sspl-plugin-av-pytest.cov")

    machine.run('cp', COVFILE_BASE, covfile, timeout=5)

    pytest_task_with_env(machine, environment={
        'COVFILE': covfile,
        'COVSRCDIR': COVSRCDIR,
    })

    machine.output_artifact(COVERAGE_DIR, output=machine.outputs.covfile)


@tap.timeout(task_timeout=TASK_TIMEOUT)
def bullseye_coverage_robot_task(machine: tap.Machine, include_tag: str, branch_name: str):
    install_requirements(machine)

    machine.run('rm', '-rf', COVERAGE_DIR, timeout=60)
    machine.run('mkdir', '-p', COVERAGE_DIR, timeout=5)

    # use /tmp so that it's accessible to all users/processes
    covfile = os.path.join("/tmp", "sspl-plugin-av-robot-" + include_tag + ".cov")
    machine.run('cp', COVFILE_BASE, covfile, timeout=5)
    machine.run('chmod', '666', covfile, timeout=5)

    # set bullseye environment in a file, so that daemons pick up the settings too
    machine.run('bash', os.path.join(BULLSEYE_SCRIPT_DIR, "createBullseyeCoverageEnv.sh"),
                covfile, COVSRCDIR, timeout=120)

    exception = None
    try:
        robot_task_with_env(machine,
                            include_tag=include_tag,
                            branch_name=branch_name,
                            environment={
                                'COVFILE': covfile,
                                'COVSRCDIR': COVSRCDIR,
                            },
                            machine_name="coverage")
    except tap.exceptions.PipelineProcessExitNonZeroError as e:
        exception = e

    machine.run('rm', '-f', '/tmp/BullseyeCoverageEnv.txt', timeout=5)

    machine.run('cp', covfile, COVERAGE_DIR, timeout=5)
    machine.output_artifact(COVERAGE_DIR, output=machine.outputs.covfile)

    if exception is not None:
        raise exception


def bullseye_upload(machine: tap.Machine, name, branch_name: str, covfile=None):
    suffix = get_suffix(branch_name)
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
                },
                timeout=120)

    return covfile


def bullseye_merge(machine: tap.Machine, output_name, *input_names):
    coverage_results_dir = os.path.join(RESULTS_DIR, 'coverage')

    output_file = os.path.join(coverage_results_dir, output_name + ".cov")
    input_files = {os.path.join(coverage_results_dir, input_name + ".cov") for input_name in input_names}

    machine.run('covmerge', '--create', '--file', output_file, *input_files, timeout=600)

    return output_file


def bullseye_test_coverage(machine: tap.Machine, covfile, branch_name: str):
    coverage_results_dir = os.path.join(RESULTS_DIR, 'coverage')

    coverage_normalise_json = os.path.join(coverage_results_dir, "test_coverage.json")
    coverage_min_function = 70
    coverage_min_condition = 70
    test_coverage_script = machine.inputs.bazel_tools / 'tools' / 'src' / 'bullseye' / 'test_coverage.py'
    test_coverage_args = [
        'python', '-u', str(test_coverage_script),
        covfile, '--output', coverage_normalise_json,
        '--min-function', str(coverage_min_function),
        '--min-condition', str(coverage_min_condition)
    ]
    if branch_name == 'develop':
        test_coverage_args += ['--upload', '--upload-job', 'UnifiedPipelines/linuxep/sspl-plugin-anti-virus']
    machine.run(*test_coverage_args, timeout=1200)


def bullseye_coverage_combine_task(machine: tap.Machine, include_tag: str, branch_name: str):
    install_requirements(machine)

    # str(machine.inputs.covfiles) = /opt/test/inputs/covfiles
    # /opt/test/inputs/covfiles/av_plugin.testing.coverage.ubuntu1804_bullseye_component/machine/sspl-plugin-av-pytest.cov
    machine.run('ls', '-Rl', machine.inputs.covfiles, timeout=10)

    coverage_results_dir = os.path.join(RESULTS_DIR, 'coverage')
    machine.run('rm', '-rf', coverage_results_dir, timeout=60)
    machine.run('mkdir', coverage_results_dir, timeout=5)

    try:
        # copy all the covfiles to coverage_results_dir
        machine.run('cp', COVFILE_UNITTEST, coverage_results_dir, timeout=5)
        machine.run('find', machine.inputs.covfiles, '-type', 'f', '-exec', 'cp', r'\{\}', coverage_results_dir, r'\;',
                    timeout=60)

        machine.run('ls', '-Rl', coverage_results_dir,
                    timeout=60)

        # individual coverage
        bullseye_upload(machine, 'sspl-plugin-av-unit', branch_name)
        bullseye_upload(machine, 'sspl-plugin-av-pytest', branch_name)
        robot_covfiles = []
        for include in include_tag.split():
            covfile = 'sspl-plugin-av-robot-' + include
            bullseye_upload(machine, covfile, branch_name)
            robot_covfiles.append(covfile)

        # robot coverage
        bullseye_merge(machine,
                       'sspl-plugin-av-robot',
                       *robot_covfiles)
        bullseye_upload(machine, 'sspl-plugin-av-robot', branch_name)

        # combined coverage
        bullseye_merge(machine,
                       'sspl-plugin-av-combined',
                       'sspl-plugin-av-unit', 'sspl-plugin-av-pytest', 'sspl-plugin-av-robot')
        covfile_combined = bullseye_upload(machine, 'sspl-plugin-av-combined', branch_name)

        # check (and report) overall coverage
        bullseye_test_coverage(machine, covfile_combined, branch_name)
    finally:
        machine.output_artifact(coverage_results_dir, 'coverage')


def decide_whether_to_run_cppcheck(parameters: tap.Parameters, context: tap.PipelineContext) -> bool:
    if parameters.force_run_cppcheck != 'false':
        return True
    if parameters.inhibit_run_cppcheck != 'false':
        return False
    branch = context.branch
    return not branch.startswith("release/")


def run_av_coverage_tests(stage, context, av_coverage_build, mode, parameters):

    # robot_task_with_env will parse this string
    default_include_tags = "TAP_PARALLEL1,TAP_PARALLEL2,TAP_PARALLEL3,TAP_PARALLEL4,TAP_PARALLEL5,TAP_PARALLEL6"
    include_tag = parameters.include_tag or default_include_tags

    # Not sure why it's different to all the other plugins?
    av_coverage_machine = "ubuntu1804_x64_server_en_us"

    with stage.parallel('av_coverage'):
        with stage.parallel('coverage'):
            coverage_inputs = get_inputs(context, av_coverage_build, coverage=True, bazel=False)

            with stage.parallel('pytest'):
                machine_bullseye_pytest = tap.Machine(av_coverage_machine,
                                                      inputs=coverage_inputs,
                                                      outputs={'covfile': TaskOutput('covfiles')},
                                                      platform=tap.Platform.Linux)
                stage.task(task_name='component',
                           func=bullseye_coverage_pytest_task,
                           machine=machine_bullseye_pytest)

            with stage.parallel('robot'):
                for include in include_tag.split():
                    machine_bullseye_robot = tap.Machine(av_coverage_machine,
                                                         inputs=coverage_inputs,
                                                         outputs={'covfile': TaskOutput('covfiles')},
                                                         platform=tap.Platform.Linux)
                    stage.task(task_name=include,
                               func=bullseye_coverage_robot_task,
                               machine=machine_bullseye_robot,
                               include_tag=include,
                               branch_name=context.branch)

            combine_inputs = coverage_inputs
            combine_inputs['covfiles'] = TaskOutput('covfiles')
            machine_bullseye_combine = \
                tap.Machine('ubuntu1804_x64_server_en_us',
                            inputs=combine_inputs,
                            platform=tap.Platform.Linux)
            stage.task(task_name='combine',
                       func=bullseye_coverage_combine_task,
                       machine=machine_bullseye_combine,
                       include_tag=include_tag,
                       branch_name=context.branch)


def run_av_tests(stage, context, builds, mode, parameters, coverage_build=None, bazel=True):
    # run_tests: bool = parameters.run_tests != 'false'

    robot_args = get_robot_args(parameters)

    # robot_task_with_env will parse this string
    default_include_tags = "TAP_PARALLEL1,TAP_PARALLEL2,TAP_PARALLEL3,TAP_PARALLEL4,TAP_PARALLEL5,TAP_PARALLEL6"
    include_tag = parameters.include_tag or default_include_tags

    with stage.parallel('av_test'):
        with stage.parallel('TA'):
            test_inputs = {}
            for arch, build in builds.items():
                if build is not None:
                    print("Running tests for", arch, "from build", build.arch, "type", build.build_type)
                    test_arch = common.test_arch(arch)
                    test_inputs[test_arch] = get_inputs(context, build, bazel=bazel, arch=arch)

            # Robot before pytest, since pytest is quick
            with stage.parallel('robot'):
                if robot_args:
                    with stage.parallel("integration"):
                        for (name, machine) in get_test_machines(test_inputs, parameters):
                            stage.task(task_name=name, func=robot_task, machine=machine,
                                       robot_args=robot_args, include_tag="", branch_name=context.branch,
                                       machine_name=name)
                else:
                    for include in include_tag.split(","):
                        with stage.parallel(include):
                            for (name, machine) in get_test_machines(test_inputs, parameters):
                                stage.task(task_name=name, func=robot_task, machine=machine,
                                           include_tag=include, branch_name=context.branch, robot_args="",
                                           machine_name=name)

            with stage.parallel('pytest'):
                with stage.parallel('component'):
                    for (name, machine) in get_test_machines(test_inputs, parameters):
                        stage.task(task_name=name, func=pytest_task, machine=machine)

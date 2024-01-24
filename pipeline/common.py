# Copyright 2023 Sophos Limited. All rights reserved.

import os.path
import time

import tap.v1 as tap
from typing import Any, Callable, Dict, List
from dotdict import DotDict

import xml.etree.ElementTree as ET
import random

X86_64 = "x86_64"
x86_64 = X86_64
x64 = X86_64

ARM64 = "arm64"
arm64 = ARM64

SDDS = "sdds"
SDDS999 = "sdds999"

INDEPENDENT_MODE = 'independent'
RELEASE_MODE = 'release'
COVERAGE_MODE = 'coverage'
FUZZER_MODE = 'fuzzers'

DEFAULT_TEST_TIMEOUT_MINUTES = 90
COVERAGE_TEST_TIMEOUT_MULTIPLIER = 4
TEST_TASK_TIMEOUT_MINUTES = COVERAGE_TEST_TIMEOUT_MULTIPLIER * DEFAULT_TEST_TIMEOUT_MINUTES

COVERAGE_TEMPLATE = "centos7_x64_aws_server_en_us"
FUZZ_TEMPLATE = "ubuntu2204_x64_aws_server_en_us"

def get_robot_args(parameters):
    # Add args to pass env vars to RobotFramework.py call in test runs
    robot_args_list = []
    # if mode == DEBUG_MODE:
    #     robot_args_list.append("DEBUG=true")
    if parameters.robot_test:
        robot_args_list.append("--test=" + parameters.robot_test)
    if parameters.robot_suite:
        robot_args_list.append("--suite=" + parameters.robot_suite)

    return robot_args_list


def get_suffix(branch_name: str):
    if branch_name == "develop":
        return ""
    return "-" + branch_name


def unified_artifact(context: tap.PipelineContext, component: str, branch: str, sub_directory: str):
    """Return an input artifact from an unified pipeline build"""
    artifact = context.artifact.from_component(component, branch, org='', storage='esg-build-tested')
    # Using the truediv operator to set the sub directory forgets the storage option
    artifact.sub_directory = sub_directory
    return artifact


def get_package_version(release_pkg: str) -> str:
    """ Read version from package.xml """
    package_tree = ET.parse(release_pkg)
    package_node = package_tree.getroot()
    return package_node.attrib['version']


def truthy(value, parameter_name, default_value):
    if value in (True, "true", 1, "1", "T", "True"):
        return True
    if value in (False, "false", 0, "0", "F", "False"):
        return False
    if value is None:
        return default_value
    print(f"{parameter_name} is not a good truthy value: {value}, assuming {default_value}")
    return default_value


def x86_64_enabled(parameters):
    arch = parameters.architectures
    return arch in ('both separately', 'both unified', 'x86_64 only', None)


def arm64_enabled(parameters):
    arch = parameters.architectures
    return arch in ('both separately', 'both unified', 'arm64 only', None)


def do_unified_build(parameters):
    arch = parameters.architectures
    return arch in ('both unified',)


def get_random_machines(x64_count: int, arm64_count: int, x64platforms: dict, arm64platforms: dict) -> dict:
    assert (x64_count <= len(x64platforms) / 2)
    assert (arm64_count <= len(arm64platforms) / 2)

    test_machines = {"x64": {}, "arm64": {}}
    while len(test_machines["x64"]) < x64_count:
        x64machine = random.choice(list(x64platforms.keys()))
        if test_machines["x64"].get(x64machine) is None:
            test_machines["x64"][x64machine] = x64platforms.get(x64machine)

    while len(test_machines["arm64"]) < arm64_count:
        arm64machine = random.choice(list(arm64platforms.keys()))
        if test_machines["x64"].get(arm64machine) is None and test_machines["arm64"].get(arm64machine) is None:
            test_machines["arm64"][arm64machine] = arm64platforms.get(arm64machine)

    print(f"Randomly selected test machines: {test_machines}")
    return test_machines


def get_test_machines(build: str, parameters: DotDict):
    if build == "linux_x64_bullseye":
        return [COVERAGE_TEMPLATE]
    elif build == "linux_x64_clang":
        return [FUZZ_TEMPLATE]

    available_x64_environments = {
        'amazonlinux2': 'amzlinux2_x64_server_en_us',
        'amazonlinux2023': 'amzlinux2023_x64_server_en_us',
        'centos79': 'centos7_x64_aws_server_en_us',
        'centos8stream': 'centos8stream_x64_aws_server_en_us',
        'centos9stream': 'centos9stream_x64_aws_server_en_us',
        'debian10': 'debian10_x64_aws_server_en_us',
        'debian11': 'debian11_x64_aws_server_en_us',
        'oracle7': 'oracle79_x64_aws_server_en_us',
        'oracle8': 'oracle87_x64_aws_server_en_us',
        'rhel7': 'rhel79_x64_aws_server_en_us',
        'rhel8': 'rhel87_x64_aws_server_en_us',
        'rhel9': 'rhel91_x64_aws_server_en_us',
        'sles12': 'sles12_x64_sp5_aws_server_en_us',
        'sles15': 'sles15_x64_sp5_aws_server_en_us',
        'ubuntu1804': 'ubuntu1804_x64_aws_server_en_us',
        'ubuntu2004': 'ubuntu2004_x64_aws_server_en_us',
        'ubuntu2204': 'ubuntu2204_x64_aws_server_en_us',
    }

    available_arm64_environments = {
        'amazonlinux2': 'amzlinux2_arm64_server_en_us',
        'amazonlinux2023': 'amzlinux2023_arm64_server_en_us',
        'centos8stream': 'centos8stream_arm64_server_en_us',
        'centos9stream': 'centos9stream_arm64_server_en_us',
        'debian10': 'debian10_arm64_server_en_us',
        'debian11': 'debian11_arm64_server_en_us',
        'rhel8': 'rhel87_arm64_server_en_us',
        'rhel9': 'rhel91_arm64_server_en_us',
        'sles15': 'sles15_arm64_sp5_server_en_us',
        'ubuntu1804': 'ubuntu1804_arm64_server_en_us',
        'ubuntu2004': 'ubuntu2004_arm64_server_en_us',
        'ubuntu2204': 'ubuntu2204_arm64_server_en_us',
    }

    run_single_x64_environments = {
        'centos79': 'centos7_x64_aws_server_en_us',
    }

    run_four_x64_environments = {
        'centos79': 'centos7_x64_aws_server_en_us',
        'debian11': 'debian11_x64_aws_server_en_us',
        'sles15': 'sles15_x64_sp5_aws_server_en_us',
    }

    run_four_arm64_environments = {
        'amazonlinux2023': 'amzlinux2023_arm64_server_en_us',
    }

    test_platform = parameters.test_platform or "run_all"

    # Process test_platform
    test_environments = {"x64": {}, "arm64": {}}
    if test_platform == "run_all":
        test_environments["x64"] = available_x64_environments
        test_environments["arm64"] = available_arm64_environments
    elif test_platform == "run_overnight":
        x64_count = 4
        arm64_count = 2
        test_environments = get_random_machines(x64_count, arm64_count, available_x64_environments,
                                                available_arm64_environments)
    elif test_platform == "run_single":
        test_environments["x64"] = run_single_x64_environments
    elif test_platform == "run_four":
        test_environments["x64"] = run_four_x64_environments
        test_environments["arm64"] = run_four_arm64_environments

    platform = 'amazonlinux2'
    if parameters.run_amazon_2 == "force_run":
        test_environments["x64"][platform] = available_x64_environments.get(platform)
        test_environments["arm64"][platform] = available_arm64_environments.get(platform)
    elif parameters.run_amazon_2 == "dont_run":
        test_environments["x64"].pop(platform, None)
        test_environments["arm64"].pop(platform, None)

    platform = 'amazonlinux2023'
    if parameters.run_amazon_2023 == "force_run":
        test_environments["x64"][platform] = available_x64_environments.get(platform)
        test_environments["arm64"][platform] = available_arm64_environments.get(platform)
    elif parameters.run_amazon_2023 == "dont_run":
        test_environments["x64"].pop(platform, None)
        test_environments["arm64"].pop(platform, None)

    platform = 'centos79'
    if parameters.run_centos_7 == "force_run":
        test_environments["x64"][platform] = available_x64_environments.get(platform)
    elif parameters.run_centos_7 == "dont_run":
        test_environments["x64"].pop(platform, None)

    platform = 'centos8stream'
    if parameters.run_centos_stream_8 == "force_run":
        test_environments["x64"][platform] = available_x64_environments.get(platform)
        test_environments["arm64"][platform] = available_arm64_environments.get(platform)
    elif parameters.run_centos_stream_8 == "dont_run":
        test_environments["x64"].pop(platform, None)
        test_environments["arm64"].pop(platform, None)

    platform = 'centos9stream'
    if parameters.run_centos_stream_9 == "force_run":
        test_environments["x64"][platform] = available_x64_environments.get(platform)
        test_environments["arm64"][platform] = available_arm64_environments.get(platform)
    elif parameters.run_centos_stream_9 == "dont_run":
        test_environments["x64"].pop(platform, None)
        test_environments["arm64"].pop(platform, None)

    platform = 'debian10'
    if parameters.run_debian_10 == "force_run":
        test_environments["x64"][platform] = available_x64_environments.get(platform)
        test_environments["arm64"][platform] = available_arm64_environments.get(platform)
    elif parameters.run_debian_10 == "dont_run":
        test_environments["x64"].pop(platform, None)
        test_environments["arm64"].pop(platform, None)

    platform = 'debian11'
    if parameters.run_debian_11 == "force_run":
        test_environments["x64"][platform] = available_x64_environments.get(platform)
        test_environments["arm64"][platform] = available_arm64_environments.get(platform)
    elif parameters.run_debian_11 == "dont_run":
        test_environments["x64"].pop(platform, None)
        test_environments["arm64"].pop(platform, None)

    platform = 'oracle7'
    if parameters.run_oracle_7 == "force_run":
        test_environments["x64"][platform] = available_x64_environments.get(platform)
    elif parameters.run_oracle_7 == "dont_run":
        test_environments["x64"].pop(platform, None)

    platform = 'oracle8'
    if parameters.run_oracle_8 == "force_run":
        test_environments["x64"][platform] = available_x64_environments.get(platform)
    elif parameters.run_oracle_8 == "dont_run":
        test_environments["x64"].pop(platform, None)

    platform = 'rhel7'
    if parameters.run_rhel_7 == "force_run":
        test_environments["x64"][platform] = available_x64_environments.get(platform)
    elif parameters.run_rhel_7 == "dont_run":
        test_environments["x64"].pop(platform, None)

    platform = 'rhel8'
    if parameters.run_rhel_8 == "force_run":
        test_environments["x64"][platform] = available_x64_environments.get(platform)
        test_environments["arm64"][platform] = available_arm64_environments.get(platform)
    elif parameters.run_rhel_8 == "dont_run":
        test_environments["x64"].pop(platform, None)
        test_environments["arm64"].pop(platform, None)

    platform = 'rhel9'
    if parameters.run_rhel_9 == "force_run":
        test_environments["x64"][platform] = available_x64_environments.get(platform)
        test_environments["arm64"][platform] = available_arm64_environments.get(platform)
    elif parameters.run_rhel_9 == "dont_run":
        test_environments["x64"].pop(platform, None)
        test_environments["arm64"].pop(platform, None)

    platform = 'sles12'
    if parameters.run_sles_12 == "force_run":
        test_environments["x64"][platform] = available_x64_environments.get(platform)
    elif parameters.run_sles_12 == "dont_run":
        test_environments["x64"].pop(platform, None)

    platform = 'sles15'
    if parameters.run_sles_15 == "force_run":
        test_environments["x64"][platform] = available_x64_environments.get(platform)
        test_environments["arm64"][platform] = available_arm64_environments.get(platform)
    elif parameters.run_sles_15 == "dont_run":
        test_environments["x64"].pop(platform, None)
        test_environments["arm64"].pop(platform, None)

    platform = 'ubuntu1804'
    if parameters.run_ubuntu_18_04 == "force_run":
        test_environments["x64"][platform] = available_x64_environments.get(platform)
        test_environments["arm64"][platform] = available_arm64_environments.get(platform)
    elif parameters.run_ubuntu_18_04 == "dont_run":
        test_environments["x64"].pop(platform, None)
        test_environments["arm64"].pop(platform, None)

    platform = 'ubuntu2004'
    if parameters.run_ubuntu_20_04 == "force_run":
        test_environments["x64"][platform] = available_x64_environments.get(platform)
        test_environments["arm64"][platform] = available_arm64_environments.get(platform)
    elif parameters.run_ubuntu_20_04 == "dont_run":
        test_environments["x64"].pop(platform, None)
        test_environments["arm64"].pop(platform, None)

    platform = 'ubuntu2204'
    if parameters.run_ubuntu_22_04 == "force_run":
        test_environments["x64"][platform] = available_x64_environments.get(platform)
        test_environments["arm64"][platform] = available_arm64_environments.get(platform)
    elif parameters.run_ubuntu_22_04 == "dont_run":
        test_environments["x64"].pop(platform, None)
        test_environments["arm64"].pop(platform, None)

    arch = build.split("_")[1]
    return test_environments[arch].values()


def package_install(machine: tap.Machine, *install_args: str):
    confirmation_arg = "-y"
    if machine.run('which', 'apt-get', return_exit_code=True, timeout=5) == 0:
        pkg_installer = "apt-get"
    elif machine.run('which', 'yum', return_exit_code=True, timeout=5) == 0:
        pkg_installer = "yum"
    else:
        pkg_installer = "zypper"
        confirmation_arg = "--non-interactive"

    for _ in range(20):
        if machine.run(pkg_installer, confirmation_arg, 'install', *install_args,
                       log_mode=tap.LoggingMode.ON_ERROR,
                       return_exit_code=True,
                       timeout=600) == 0:
            break
        else:
            time.sleep(3)


def pip_install(machine: tap.Machine, *install_args: str):
    """Installs python packages onto a TAP machine"""
    pip_index = os.environ.get('TAP_PIP_INDEX_URL')
    pip_index_args = ['--index-url', pip_index] if pip_index else []
    pip_index_args += ["--no-cache-dir",
                       "--progress-bar", "off",
                       "--disable-pip-version-check",
                       "--default-timeout", "120"]
    machine.run("pip3", 'install', "pip", "--upgrade", *pip_index_args,
                log_mode=tap.LoggingMode.ON_ERROR, timeout=300)
    machine.run("pip3", '--log', '/opt/test/logs/pip.log',
                'install', *install_args, *pip_index_args,
                log_mode=tap.LoggingMode.ON_ERROR, timeout=300)


def python(machine: tap.Machine):
    return "python3"


def pip(machine: tap.Machine):
    return "pip3"


def test_arch(arch):
    return {
        "x86_64": "x64"
    }.get(arch, arch)


def get_os_packages(machine: tap.Machine):
    common = [
        "git",
        "openssl",
        "rsync",
        "strace",
        "unzip",
        "zip",
    ]
    # X64
    if machine.template == "amzlinux2_x64_server_en_us":
        return common
    elif machine.template == "amzlinux2023_x64_server_en_us":
        return common + ["iptables-services"]
    elif machine.template == "centos7_x64_aws_server_en_us":
        return common
    elif machine.template == "centos8stream_x64_aws_server_en_us":
        return common + ["iptables-services"]
    elif machine.template == "centos9stream_x64_aws_server_en_us":
        return common + ["iptables-services"]
    elif machine.template == "debian10_x64_aws_server_en_us":
        return common
    elif machine.template == "debian11_x64_aws_server_en_us":
        return common
    elif machine.template == "oracle79_x64_aws_server_en_us":
        return common + ["iptables-services"]
    elif machine.template == "oracle87_x64_aws_server_en_us":
        return common
    elif machine.template == "rhel79_x64_aws_server_en_us":
        return common
    elif machine.template == "rhel87_x64_aws_server_en_us":
        return common + ["iptables-services"]
    elif machine.template == "rhel91_x64_aws_server_en_us":
        return common + ["iptables-services"]
    elif machine.template == "sles12_x64_sp5_aws_server_en_us":
        return common + ["libcap-progs", "curl"]
    elif machine.template == "sles15_x64_sp5_aws_server_en_us":
        return common + ["libcap-progs"]
    elif machine.template == "ubuntu1804_x64_aws_server_en_us":
        return common
    elif machine.template == "ubuntu2004_x64_aws_server_en_us":
        return common
    elif machine.template == "ubuntu2204_x64_aws_server_en_us":
        return common
    # ARM64
    elif machine.template == "amzlinux2_arm64_server_en_us":
        return common
    elif machine.template == "amzlinux2023_arm64_server_en_us":
        return common + ["iptables-services"]
    elif machine.template == "centos8stream_arm64_server_en_us":
        return common + ["iptables-services"]
    elif machine.template == "centos9stream_arm64_server_en_us":
        return common + ["iptables-services"]
    elif machine.template == "debian10_arm64_server_en_us":
        return common
    elif machine.template == "debian11_arm64_server_en_us":
        return common
    elif machine.template == "rhel87_arm64_server_en_us":
        return common + ["iptables-services"]
    elif machine.template == "rhel91_arm64_server_en_us":
        return common + ["iptables-services"]
    elif machine.template == "sles15_arm64_sp5_server_en_us":
        return common + ["libcap-progs"]
    elif machine.template == "ubuntu1804_arm64_server_en_us":
        return common
    elif machine.template == "ubuntu2004_arm64_server_en_us":
        return common
    elif machine.template == "ubuntu2204_arm64_server_en_us":
        return common
    else:
        raise Exception(f"Unknown template {machine.template}")


def is_coverage_enabled(parameters):
    return parameters.mode == COVERAGE_MODE or truthy(parameters.code_coverage, "CODE_COVERAGE", False)

def is_fuzzing_enabled(parameters):
    return parameters.mode == FUZZER_MODE or truthy(parameters.build_fuzzers, "BUILD_FOR_FUZZING", False)

def stage_task(
    stage: tap.Root,
    coverage_tasks: List[str],
    group_name: str,
    task_name: str,
    build: str,
    func: Callable,
    **kwargs,
):
    coverage = build == "linux_x64_bullseye"
    if coverage:
        coverage_tasks.append(f"{group_name}.{task_name}")
    stage.task(
        task_name=task_name,
        func=func,
        **kwargs,
    )


@tap.timeout(task_timeout=180)
def run_coverage_results_task(machine: tap.Machine, branch: str):
    machine.run("python3", "/opt/test/inputs/bullseye_scripts/final_coverage_steps.py", branch, timeout=180 * 60)
    machine.output_artifact("/opt/test/coverage", "coverage_results")


def stage_coverage_results_task(
    stage: tap.Root,
    context: tap.PipelineContext,
    component: tap.Component,  # pylint: disable=unused-argument
    parameters: DotDict,  # pylint: disable=unused-argument
    outputs: Dict[str, Any],
    coverage_tasks: List[str],
):
    if len(coverage_tasks) == 0:
        return

    stage_after = [f"linux_mono_repo.products.testing.{t}" for t in coverage_tasks]

    cov_output = outputs["bullseye"]

    with stage.parallel("coverage_results", after=stage_after):
        template_name = COVERAGE_TEMPLATE
        inputs = dict(
            coverage_unit=cov_output / "coverage_unit",  # use monorepo_unit.cov file
            coverage=tap.TaskOutput("covfiles"),
            bullseye_scripts=context.artifact.from_folder("./build/bullseye"),
            bazel_tools=context.artifact.from_component("em.esg", "develop", org="", storage="esg-build-tested")
            / "build/bazel-tools",
        )
        machine = tap.Machine(template_name, inputs=inputs, platform=tap.Platform.Linux)
        stage.task(task_name=template_name, func=run_coverage_results_task, machine=machine, branch=context.branch)


def get_build_output_for_arch(outputs: dict, name: str):
    if name == "linux_x64_rel":
        return outputs.get(x86_64, None)
    if name == "linux_arm64_rel":
        return outputs.get(arm64, None)
    if name == "linux_x64_bullseye":
        return outputs.get("bullseye", None)
    return None


def stage_tap_machine(
    template: str, inputs: "DotDict | Dict[str, Any]", build_outputs: Dict[str, Any], build: str, **kwargs
):
    outputs = kwargs.pop("outputs", {})
    if build == "linux_x64_bullseye":
        inputs["coverage"] = get_build_output_for_arch(build_outputs, "linux_x64_bullseye") / "coverage_none"
        outputs["coverage"] = tap.TaskOutput("covfiles")
    return tap.Machine(template=template, inputs=inputs, outputs=outputs, platform=tap.Platform.Linux, **kwargs)


class CoverageWrapper:
    def __init__(self, machine: tap.Machine, covfile: "str | None"):
        self.covdir = "/opt/test/inputs/coverage"
        self.machine = machine
        self.covfile = os.path.join(self.covdir, covfile) if covfile else ""
        self.enabled = bool(covfile)

    def scale_timeout(self, unscaled_timeout_seconds=None):
        timeout_seconds = unscaled_timeout_seconds if unscaled_timeout_seconds else 60 * DEFAULT_TEST_TIMEOUT_MINUTES
        timeout_seconds *= COVERAGE_TEST_TIMEOUT_MULTIPLIER if self.enabled else 1
        return timeout_seconds

    def __enter__(self):
        if self.enabled:
            # "/tmp/BullseyeCoverageEnv.txt" is a special location that bullseye checks for config values
            # We can set the COVFILE env var here so that all instrumented processes know where it is.
            self.machine.run("echo", f"COVFILE={self.covfile}", ">", "/tmp/BullseyeCoverageEnv.txt", timeout=5)
            self.machine.run("chmod", "0644", "/tmp/BullseyeCoverageEnv.txt", timeout=5)
            # Make sure that any product process can update the cov file, no matter the running user.
            self.machine.run("chmod", "0666", self.covfile, timeout=5)
        return self

    def __exit__(self, *_):
        if not self.enabled:
            return False  # not enabled: do not suppress exceptions

        self.machine.output_artifact(self.covdir, output=self.machine.outputs.coverage)
        return True  # coverage enabled: suppress exceptions in wrapped code


def install_requirements(machine: tap.Machine, os_packages: List[str]):
    """install python lib requirements"""

    try:
        machine.run("which", python(machine), return_exit_code=True, timeout=5)
        machine.run("which", pip(machine), return_exit_code=True, timeout=5)
        machine.run(python(machine), "-V", return_exit_code=True, timeout=5)
        machine.run("which", "python", return_exit_code=True, timeout=5)
        machine.run("python", "-V", return_exit_code=True, timeout=5)
    except Exception:
        pass

    if machine.inputs.common_test_utils is None:
        raise Exception("machine.inputs.common_test_utils is not set")

    if machine.inputs.test_scripts is None:
        raise Exception("machine.inputs.test_scripts is not set")

    pip_install(machine, "-r", machine.inputs.test_scripts / "requirements.txt")
    install_command = ["bash", machine.inputs.common_test_utils / "install_os_packages.sh"] + os_packages
    machine.run(*install_command, timeout=600)


def run_robot_tests(
    machine: tap.Machine,
    os_packages: List[str],
    timeout_seconds=None,
    robot_args=None,
):
    try:
        if not robot_args:
            robot_args = []

        platform, arch = machine.template.split("_")[:2]
        default_exclude_tags = [
            "CENTRAL",
            "MANUAL",
            "TESTFAILURE",
            "FUZZ",
            "SYSTEMPRODUCTTESTINPUT",
            f"EXCLUDE_{platform.upper()}",
            f"EXCLUDE_{arch.upper()}",
            f"EXCLUDE_{platform.upper()}_{arch.upper()}",
            "DISABLED",
            "STRESS",
            "OSTIA",  # Needed for AV, not used anywhere else
        ]

        print(f"test scripts: {machine.inputs.test_scripts}")
        print(f"robot_args: {robot_args}")
        install_requirements(machine, os_packages)

        machine.run("mkdir", "-p", "/opt/test/coredumps", timeout=5)

        covfile = "monorepo.cov" if "coverage" in machine.inputs else None
        with CoverageWrapper(
            machine,
            covfile,
        ) as coverage:
            machine.run(
                python(machine),
                machine.inputs.test_scripts / "RobotFramework.py",
                *robot_args,
                "--exclude",
                *default_exclude_tags,
                timeout=coverage.scale_timeout(timeout_seconds),
            )

    finally:
        machine.run("python3", machine.inputs.test_scripts / "move_robot_results.py", timeout=60)
        machine.output_artifact("/opt/test/logs", "logs")
        machine.output_artifact("/opt/test/results", "results")
        machine.output_artifact("/opt/test/coredumps", "coredumps", raise_on_failure=False)


def run_pytest_tests(
    machine: tap.Machine,
    test_paths: List[str],
    os_packages: List[str],
    scripts="scripts",
    timeout_seconds=None,
    extra_pytest_args_behind_paths=None,
):
    try:
        install_requirements(machine, os_packages)

        # To run the pytest with additional verbosity add following to arguments
        # "-o", "log_cli=true"
        args = ["python3", "-u", "-m", "pytest"]
        args += ["-o", "log_cli=true"]
        args += ["--durations=0", "-v", "-s"]

        # Set TAP_PARAMETER_TEST_PATTERN environment variable to a pytest filter to narrow scope when writing tests.
        test_filter = os.getenv("TAP_PARAMETER_TEST_PATTERN")
        if bool(test_filter):
            args += ["--suppress-no-test-exit-code", "-k", test_filter]

        for test_path in test_paths:
            args.append(machine.inputs[scripts] / test_path)

        if extra_pytest_args_behind_paths:
            args += extra_pytest_args_behind_paths

        machine.run("mkdir", "-p", "/opt/test/coredumps", timeout=5)

        covfile = "monorepo.cov" if "coverage" in machine.inputs else None
        with CoverageWrapper(
            machine,
            covfile,
        ) as coverage:
            machine.run(*args, timeout=coverage.scale_timeout(timeout_seconds))
    finally:
        machine.output_artifact("/opt/test/logs", "logs")
        machine.output_artifact("/opt/test/results", "results")
        machine.output_artifact("/opt/test/coredumps", "coredumps", raise_on_failure=False)


def get_test_builds():
    return ["linux_x64_rel", "linux_arm64_rel", "linux_x64_bullseye"]
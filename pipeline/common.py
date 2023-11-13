# Copyright 2023 Sophos Limited. All rights reserved.

import os.path
import time

import tap.v1 as tap
import xml.etree.ElementTree as ET
import random

X86_64 = "x86_64"
x86_64 = X86_64
x64    = X86_64

ARM64 = "arm64"
arm64 = ARM64

INDEPENDENT_MODE = 'independent'
RELEASE_MODE = 'release'
ANALYSIS_MODE = 'analysis'
COVERAGE_MODE = 'coverage'
NINE_NINE_NINE_MODE = '999'
ZERO_SIX_ZERO_MODE = '060'
ROBOT_TEST_TIMEOUT = 5400
TASK_TIMEOUT = 120

COVERAGE_TEMPLATE = "centos7_x64_aws_server_en_us"


def get_robot_args_list(parameters, use_env_vars):
    # Add args to pass env vars to RobotFramework.py call in test runs
    robot_args_list = []
    # if mode == DEBUG_MODE:
    #     robot_args_list.append("DEBUG=true")
    if parameters.robot_test:
        if use_env_vars:
            robot_args_list.append("TEST=" + parameters.robot_test)
        else:
            robot_args_list.append("--test=" + parameters.robot_test)
    if parameters.robot_suite:
        if use_env_vars:
            robot_args_list.append("SUITE=" + parameters.robot_suite)
        else:
            robot_args_list.append("--suite=" + parameters.robot_suite)

    return robot_args_list


def get_robot_args(parameters):
    return " ".join(get_robot_args_list(parameters, use_env_vars=True))


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


def get_random_machines(machines_no: int, x64platforms: dict, arm64platforms: dict) -> dict:
    assert(machines_no <= len(x64platforms)/2)
    assert(machines_no <= len(arm64platforms)/2)

    test_machines = {"x64": {}, "arm64": {}}
    for machine in range(machines_no):
        x64_machine_found = False
        arm64_machine_found = False

        while not x64_machine_found:
            x64machine = random.choice(list(x64platforms.keys()))
            if test_machines["x64"].get(x64machine) is None and test_machines["arm64"].get(x64machine) is None:
                test_machines["x64"][x64machine] = x64platforms.get(x64machine)
                x64_machine_found = True

        while not arm64_machine_found:
            arm64machine = random.choice(list(arm64platforms.keys()))
            if test_machines["x64"].get(arm64machine) is None and test_machines["arm64"].get(arm64machine) is None:
                test_machines["arm64"][arm64machine] = arm64platforms.get(arm64machine)
                arm64_machine_found = True
    return test_machines

def get_test_machines(test_inputs, parameters, system_tests=False):
    run_tests = truthy(parameters.run_tests, "run_tests", True)
    if not run_tests and not system_tests:
        return []

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
        'sles15': 'sles15_x64_sp4_aws_server_en_us',
        'ubuntu1804': 'ubuntu1804_x64_aws_server_en_us',
        'ubuntu2004': 'ubuntu2004_x64_aws_server_en_us',
        'ubuntu2204': 'ubuntu2204_x64_aws_server_en_us'}

    available_arm64_environments = {
        'amazonlinux2': 'amzlinux2_arm64_server_en_us',
        'amazonlinux2023': 'amzlinux2023_arm64_server_en_us',
        'centos8stream': 'centos8stream_arm64_server_en_us',
        'centos9stream': 'centos9stream_arm64_server_en_us',
        'debian10': 'debian10_arm64_server_en_us',
        'debian11': 'debian11_arm64_server_en_us',
        'rhel8': 'rhel87_arm64_server_en_us',
        'rhel9': 'rhel91_arm64_server_en_us',
        'sles15': 'sles15_arm64_sp4_server_en_us',
        'ubuntu1804': 'ubuntu1804_arm64_server_en_us',
        'ubuntu2004': 'ubuntu2004_arm64_server_en_us',
        'ubuntu2204': 'ubuntu2204_arm64_server_en_us'}

    #Process test_platform
    test_environments = {"x64": {}, "arm64": {}}
    if parameters.test_platform == "run_all":
        test_environments["x64"] = available_x64_environments
        test_environments["arm64"] = available_arm64_environments
    elif parameters.test_platform != "run_none":
        platform_count = 0
        if parameters.test_platform == "run_single":
            platform_count = 1
        elif parameters.test_platform == "run_four":
            platform_count = 4
        test_environments = get_random_machines(platform_count, available_x64_environments, available_arm64_environments)


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

    if len(test_environments) == 0 and not system_tests:
        return []

    if test_inputs.get("arm64", None) is None or not arm64_enabled(parameters):
        test_environments.pop("arm64", None)
    else:
        assert "arm64" in test_inputs
        assert test_inputs["arm64"] is not None

    if test_inputs.get("x64", None) is None or not x86_64_enabled(parameters):
        test_environments.pop("x64", None)
    else:
        assert "x64" in test_inputs
        assert test_inputs["x64"] is not None

    ret = []
    for arch, environments in test_environments.items():
        if test_inputs[arch] is None:
            print(f"No test_input for {arch}")
            continue

        for name, image in environments.items():
            ret.append((
                f"{arch}_{name}",
                tap.Machine(image, inputs=test_inputs[arch], platform=tap.Platform.Linux)
            ))
    return ret


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


def test_arch(arch):
    return {
        "x86_64": "x64"
    }.get(arch, arch)

def get_os_packages(machine: tap.Machine):
    # TODO: LINUXDAR-7234: Remove "make" from list once fixed
    common = [
        "git",
        "make",
        "openssl",
        "rsync",
        "unzip",
    ]
    # X64
    if machine.template == "amzlinux2_x64_server_en_us":
        return common
    elif machine.template == "amzlinux2023_x64_server_en_us":
        return common + ["openssl-perl"]
    elif machine.template == "centos7_x64_aws_server_en_us":
        return common
    elif machine.template == "centos8stream_x64_aws_server_en_us":
        return common + ["openssl-perl"]
    elif machine.template == "centos9stream_x64_aws_server_en_us":
        return common + ["openssl-perl"]
    elif machine.template == "debian10_x64_aws_server_en_us":
        return common
    elif machine.template == "debian11_x64_aws_server_en_us":
        return common
    elif machine.template == "oracle79_x64_aws_server_en_us":
        return common
    elif machine.template == "oracle87_x64_aws_server_en_us":
        return common + ["openssl-perl"]
    elif machine.template == "rhel79_x64_aws_server_en_us":
        return common
    elif machine.template == "rhel87_x64_aws_server_en_us":
        return common + ["openssl-perl"]
    elif machine.template == "rhel91_x64_aws_server_en_us":
        return common + ["openssl-perl"]
    elif machine.template == "sles12_x64_sp5_aws_server_en_us":
        return common + ["libcap-progs", "curl"]
    elif machine.template == "sles15_x64_sp4_aws_server_en_us":
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
        return common + ["openssl-perl"]
    elif machine.template == "centos8stream_arm64_server_en_us":
        return common + ["openssl-perl"]
    elif machine.template == "centos9stream_arm64_server_en_us":
        return common + ["openssl-perl"]
    elif machine.template == "debian10_arm64_server_en_us":
        return common
    elif machine.template == "debian11_arm64_server_en_us":
        return common
    elif machine.template == "rhel87_arm64_server_en_us":
        return common + ["openssl-perl"]
    elif machine.template == "rhel91_arm64_server_en_us":
        return common + ["openssl-perl"]
    elif machine.template == "sles15_arm64_sp4_server_en_us":
        return common + ["libcap-progs"]
    elif machine.template == "ubuntu1804_arm64_server_en_us":
        return common
    elif machine.template == "ubuntu2004_arm64_server_en_us":
        return common
    elif machine.template == "ubuntu2204_arm64_server_en_us":
        return common
    else:
        raise Exception(f"Unknown template {machine.template}")

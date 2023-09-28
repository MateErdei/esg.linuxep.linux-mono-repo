import os.path
import time

import tap.v1 as tap
import xml.etree.ElementTree as ET

INDEPENDENT_MODE = 'independent'
RELEASE_MODE = 'release'
ANALYSIS_MODE = 'analysis'
COVERAGE_MODE = 'coverage'
NINE_NINE_NINE_MODE = '999'
ZERO_SIX_ZERO_MODE = '060'
ROBOT_TEST_TIMEOUT = 5400
TASK_TIMEOUT = 120

COVERAGE_TEMPLATE = "centos7_x64_aws_server_en_us"


def get_robot_args(parameters):
    # Add args to pass env vars to RobotFramework.py call in test runs
    robot_args_list = []
    # if mode == DEBUG_MODE:
    #     robot_args_list.append("DEBUG=true")
    if parameters.robot_test:
        robot_args_list.append("TEST=" + parameters.robot_test)
    if parameters.robot_suite:
        robot_args_list.append("SUITE=" + parameters.robot_suite)
    robot_args = " ".join(robot_args_list)
    return robot_args


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


def get_test_machines(test_inputs, parameters):
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
        test_environments['ubuntu1804'] = 'ubuntu1804_x64_aws_server_en_us'

    if parameters.run_ubuntu_20_04 != "false":
        test_environments['ubuntu2004'] = 'ubuntu2004_x64_aws_server_en_us'

    # TODO: LINUXDAR-7306 set CIJenkins to default=true once python3.10 issues are resolved
    if parameters.run_ubuntu_22_04 != "false":
        test_environments['ubuntu2204'] = 'ubuntu2204_x64_aws_server_en_us'

    ret = []
    for name, image in test_environments.items():
        ret.append((
            name,
            tap.Machine(image, inputs=test_inputs, platform=tap.Platform.Linux)
        ))
    return ret


def package_install(machine: tap.Machine, *install_args: str):
    confirmation_arg = "-y"
    if machine.run('which', 'apt-get', return_exit_code=True) == 0:
        pkg_installer = "apt-get"
    elif machine.run('which', 'yum', return_exit_code=True) == 0:
        pkg_installer = "yum"
    else:
        pkg_installer = "zypper"
        confirmation_arg = "--non-interactive"

    for _ in range(20):
        if machine.run(pkg_installer, confirmation_arg, 'install', *install_args,
                       log_mode=tap.LoggingMode.ON_ERROR,
                       return_exit_code=True) == 0:
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
                log_mode=tap.LoggingMode.ON_ERROR)
    machine.run("pip3", '--log', '/opt/test/logs/pip.log',
                'install', *install_args, *pip_index_args,
                log_mode=tap.LoggingMode.ON_ERROR)


def get_os_packages(machine: tap.Machine):
    common = [
        "git",
        "openssl",
        "rsync",
    ]
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
    else:
        raise Exception(f"Unknown template {machine.template}")

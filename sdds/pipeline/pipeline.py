import os
import time
from typing import Dict

import tap.v1 as tap
from tap._backend import Input
from tap._pipeline.tasks import ArtisanInput


def build_dev_warehouse(stage: tap.Root, name="release-package", image="Warehouse"):
    component = tap.Component(name="dev-warehouse-" + name, base_version="1.0.0")
    return stage.artisan_build(name=name,
                               component=component,
                               image=image,
                               release_package="./build/dev.xml",
                               mode=name)


def build_sdds3_warehouse(stage: tap.Root, mode="dev", image="centos79_x64_bazel_20230512"):
    component = tap.Component(name="sdds3-warehouse-" + mode, base_version="1.0.0")
    return stage.artisan_build(name=mode,
                               component=component,
                               image=image,
                               release_package="./build/dev.xml",
                               mode=mode)


def get_test_machines(test_inputs, parameters):
    test_environments = {}

    if parameters.run_amazon_2 != "false":
        test_environments["amazonlinux2"] = "amzlinux2_x64_server_en_us"

    if parameters.run_amazon_2023 != "false":
        test_environments["amazonlinux2023"] = "amzlinux2023_x64_server_en_us"

    if parameters.run_centos_7 != "false":
        test_environments["centos79"] = "centos7_x64_aws_server_en_us"

    if parameters.run_centos_stream_8 != "false":
        test_environments["centos8stream"] = "centos8stream_x64_aws_server_en_us"

    if parameters.run_centos_stream_9 != "false":
        test_environments["centos9stream"] = "centos9stream_x64_aws_server_en_us"

    if parameters.run_debian_10 != "false":
        test_environments["debian10"] = "debian10_x64_aws_server_en_us"

    if parameters.run_debian_11 != "false":
        test_environments["debian11"] = "debian11_x64_aws_server_en_us"

    if parameters.run_oracle_7 != "false":
        test_environments["oracle7"] = "oracle79_x64_aws_server_en_us"

    if parameters.run_oracle_8 != "false":
        test_environments["oracle8"] = "oracle87_x64_aws_server_en_us"

    if parameters.run_rhel_7 != "false":
        test_environments["rhel7"] = "rhel79_x64_aws_server_en_us"

    if parameters.run_rhel_8 != "false":
        test_environments["rhel8"] = "rhel87_x64_aws_server_en_us"

    if parameters.run_rhel_9 != "false":
        test_environments["rhel9"] = "rhel91_x64_aws_server_en_us"

    if parameters.run_sles_12 != "false":
        test_environments["sles12"] = "sles12_x64_sp5_aws_server_en_us"

    if parameters.run_sles_15 != "false":
        test_environments["sles15"] = "sles15_x64_sp4_aws_server_en_us"

    if parameters.run_ubuntu_18_04 != "false":
        test_environments["ubuntu1804"] = "ubuntu1804_x64_aws_server_en_us"

    if parameters.run_ubuntu_20_04 != "false":
        test_environments["ubuntu2004"] = "ubuntu2004_x64_aws_server_en_us"

    if parameters.run_ubuntu_22_04 != "false":
        test_environments["ubuntu2204"] = "ubuntu2204_x64_aws_server_en_us"

    ret = []
    for name, image in test_environments.items():
        ret.append((
            name,
            tap.Machine(image, inputs=test_inputs, platform=tap.Platform.Linux)
        ))
    return ret


def get_inputs(context: tap.PipelineContext, build: ArtisanInput, parameters: tap.Parameters) -> Dict[str, Input]:
    print(f"Build directory: {str(build)}")

    test_inputs = dict(
        test_scripts=context.artifact.from_folder("./TA"),
        repo=build / "sdds3-repo",
        launchdarkly=build / "sdds3-launchdarkly",
        thin_installer=context.artifact.from_component("linuxep.thininstaller", "develop", None, org="",
                                                       storage="esg-build-tested") / "build/output",
        base_test_utils=context.artifact.from_component("linuxep.everest-base", "feature--LINUXDAR-7531-automated-system-for-testing-downgrades-upgrades-between-versions-the-customer-can-select", None, org="",
                                                        storage="esg-build-tested") / "build/sspl-base/system_test",
        dogfood_launch_darkly=context.artifact.from_component("linuxep.sspl-warehouse", parameters.previous_dogfood_branch, None,org="",
                                                              storage="esg-build-tested") / "build/prod-sdds3-launchdarkly",
        dogfood_repo=context.artifact.from_component("linuxep.sspl-warehouse", parameters.previous_dogfood_branch, None, org="",
                                                     storage="esg-build-tested") / "build/prod-sdds3-repo",
        current_shipping_launch_darkly=context.artifact.from_component("linuxep.sspl-warehouse",
                                                                       parameters.current_shipping_branch, None, org="",
                                                                       storage="esg-build-tested") / "build/prod-sdds3-launchdarkly",
        current_shipping_repo=context.artifact.from_component("linuxep.sspl-warehouse",
                                                              parameters.current_shipping_branch, None, org="",
                                                              storage="esg-build-tested") / "build/prod-sdds3-repo",
        safestore_tools=context.artifact.from_component("core.safestore", "develop", None, org="",
                                                       storage="esg-build-tested") / "build/release/linux-x64/safestore"
    )
    return test_inputs


def python(machine: tap.Machine):
    return "python3"


def pip_install(machine: tap.Machine, *install_args: str):
    """Installs python packages onto a TAP machine"""
    pip_index = os.environ.get("TAP_PIP_INDEX_URL")
    pip_index_args = ["--index-url", pip_index] if pip_index else []
    pip_index_args += ["--no-cache-dir",
                       "--progress-bar", "off",
                       "--disable-pip-version-check",
                       "--default-timeout", "120"]
    machine.run("pip3", "install", "pip", "--upgrade", *pip_index_args,
                log_mode=tap.LoggingMode.ON_ERROR)
    machine.run("pip3", "--log", "/opt/test/logs/pip.log",
                "install", *install_args, *pip_index_args,
                log_mode=tap.LoggingMode.ON_ERROR)


def package_install(machine: tap.Machine, *install_args: str):
    confirmation_arg = "-y"
    if machine.run("which", "apt-get", return_exit_code=True) == 0:
        pkg_installer = "apt-get"
    elif machine.run("which", "yum", return_exit_code=True) == 0:
        pkg_installer = "yum"
    else:
        pkg_installer = "zypper"
        confirmation_arg = "--non-interactive"

    for _ in range(20):
        if machine.run(pkg_installer, confirmation_arg, "install", *install_args,
                       log_mode=tap.LoggingMode.ON_ERROR,
                       return_exit_code=True) == 0:
            break
        else:
            time.sleep(3)


def install_requirements(machine: tap.Machine):
    """ install python lib requirements """
    machine.run("which", python(machine))
    machine.run(python(machine), "-V")

    try:
        # Check if python2 is python on this machine
        machine.run("which", "python")
        machine.run("python", "-V")
    except Exception:
        pass

    try:
        pip_install(machine, "-r", machine.inputs.test_scripts / "requirements.txt")
        package_install(machine, "rsync")
        package_install(machine, "openssl")
    except Exception as ex:
        # the previous command will fail if user already exists. But this is not an error
        print(f"On adding installing requirements: {ex}")


def robot_task(machine: tap.Machine, parameters: tap.Parameters):
    robot_task_with_env(machine, parameters)


def robot_task_with_env(machine: tap.Machine, parameters: tap.Parameters, machine_name=None):
    if machine_name is None:
        machine_name = machine.template
    try:
        robot_exclusion_tags = ["MANUAL", "FAIL"]
        environment = {
            'CENTRAL_API_CLIENT_ID': parameters.central_api_client_id,
            'CENTRAL_API_CLIENT_SECRET': parameters.central_api_client_secret,
        }

        install_requirements(machine)
        machine.run(python(machine),
                    machine.inputs.test_scripts / "bin/RobotFramework.py",
                    *robot_exclusion_tags,
                    environment=environment,
                    timeout=3600)
    finally:
        machine.run(python(machine), machine.inputs.test_scripts / "bin/move_robot_results.py")
        machine.output_artifact("/opt/test/logs", "logs")
        machine.output_artifact("/opt/test/results", "results")


def run_tap_tests(stage: tap.Root, context: tap.PipelineContext, parameters: tap.Parameters, build):
    test_inputs = get_inputs(context, build, parameters)
    machines = get_test_machines(test_inputs, parameters)

    for template_name, machine in machines:
        stage.task(task_name=template_name, func=robot_task, machine=machine, parameters=parameters)
    return


@tap.pipeline(root_sequential=False)
def warehouse(stage: tap.Root, context: tap.PipelineContext, parameters: tap.Parameters):

    run_tests = parameters.run_tests != "false"

    with stage.parallel("build"):
        branch = context.branch

        is_release_branch = branch.startswith("release-") or branch.startswith("hotfix-")

        if is_release_branch:
            do_dev = False
            do_prod = True
        else:
            do_dev = parameters.dev != "false"
            do_prod = parameters.prod == "true"

        with stage.parallel("sdds3"):
            if do_prod:
                build_sdds3_warehouse(stage=stage, mode="prod")
                buildsdds3 = False
            if do_dev:
                buildsdds3 = build_sdds3_warehouse(stage=stage, mode="dev")
                build_sdds3_warehouse(stage=stage, mode="999")

    if run_tests and buildsdds3:
        run_tap_tests(stage, context, parameters, buildsdds3)

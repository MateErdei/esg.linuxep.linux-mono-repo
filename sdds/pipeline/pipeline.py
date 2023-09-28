import os
import json
from typing import Dict

import tap.v1 as tap
from tap._backend import Input
from tap._pipeline.tasks import ArtisanInput


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

    previous_dogfood_branch = parameters.previous_dogfood_branch
    current_shipping_branch = parameters.current_shipping_branch
    thininstaller_branch =  parameters.thininstaller_branch

    if not previous_dogfood_branch:
        previous_dogfood_branch = "release--2023-31"
    if not current_shipping_branch:
        current_shipping_branch = "release--2023.3"
    if not thininstaller_branch:
        thininstaller_branch = "develop"

    previous_dogfood_branch = previous_dogfood_branch.replace("/", "--")
    current_shipping_branch = current_shipping_branch.replace("/", "--")
    thininstaller_branch = thininstaller_branch.replace("/", "--")

    test_inputs = dict(
        test_scripts=build / "test-scripts",
        repo=build / "sdds3-repo",
        launchdarkly=build / "sdds3-launchdarkly",
        thin_installer=context.artifact.from_component("linuxep.linux-mono-repo", thininstaller_branch, None, org="",
                                                       storage="esg-build-tested") / "build/thininstaller/linux_x64_rel/thininstaller",
        dogfood_launch_darkly=context.artifact.from_component("linuxep.sspl-warehouse", previous_dogfood_branch, None,
                                                              org="",
                                                              storage="esg-build-tested") / "build/prod-sdds3-launchdarkly",
        dogfood_repo=context.artifact.from_component("linuxep.sspl-warehouse", previous_dogfood_branch, None, org="",
                                                     storage="esg-build-tested") / "build/prod-sdds3-repo",
        current_shipping_launch_darkly=context.artifact.from_component("linuxep.sspl-warehouse",
                                                                       current_shipping_branch, None, org="",
                                                                       storage="esg-build-tested") / "build/prod-sdds3-launchdarkly",
        current_shipping_repo=context.artifact.from_component("linuxep.sspl-warehouse",
                                                              current_shipping_branch, None, org="",
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


def get_os_packages(machine: tap.Machine):
    common = [
        "openssl",  # For generating certs
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

        os_packages = get_os_packages(machine)
        install_command = ["bash", machine.inputs.test_scripts / "SupportFiles/install_os_packages.sh"] + os_packages
        machine.run(*install_command)
    except Exception as ex:
        # the previous command will fail if user already exists. But this is not an error
        print(f"On adding installing requirements: {ex}")


def robot_task(machine: tap.Machine, parameters_json: str):
    try:
        parameters = json.loads(parameters_json)

        environment = {}
        if parameters.get("central_api_client_id"):
            environment["CENTRAL_API_CLIENT_ID"] = parameters["central_api_client_id"]
        if parameters.get("central_api_client_secret"):
            environment["CENTRAL_API_CLIENT_SECRET"] = parameters["central_api_client_secret"]
        if parameters.get("robot_suite"):
            environment["SUITE"] = parameters["robot_suite"]
        if parameters.get("robot_test"):
            environment["TEST"] = parameters["robot_test"]

        robot_exclusion_tags = ["MANUAL", "FAIL"]
        if "CENTRAL_API_CLIENT_ID" not in environment or "CENTRAL_API_CLIENT_SECRET" not in environment:
            robot_exclusion_tags.append("FIXED_VERSIONS")

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

    parameters_json = json.dumps(parameters)

    for template_name, machine in machines:
        stage.task(task_name=template_name, func=robot_task, machine=machine, parameters_json=parameters_json)
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

        if do_prod:
            build_sdds3_warehouse(stage=stage, mode="prod")
            build = None
        if do_dev:
            build = build_sdds3_warehouse(stage=stage, mode="dev")
            build_sdds3_warehouse(stage=stage, mode="999")

    with stage.parallel("test"):
        if run_tests and build:
            run_tap_tests(stage, context, parameters, build)

# Copyright 2023 Sophos Limited. All rights reserved.

import os
import json
from typing import Dict

import tap.v1 as tap
from tap._backend import Input
from tap._pipeline.tasks import ArtisanInput

from pipeline.common import get_test_machines, pip_install, get_os_packages, python

def build_sdds3_warehouse(stage: tap.Root, mode="dev", image="centos79_x64_bazel_20230512"):
    component = tap.Component(name="sdds3-warehouse-" + mode, base_version="1.0.0")
    return stage.artisan_build(name=mode,
                               component=component,
                               image=image,
                               release_package="./sdds/build/dev.xml",
                               mode=mode)

def get_inputs(context: tap.PipelineContext, build: ArtisanInput, parameters: tap.Parameters) -> Dict[str, Input]:
    print(f"Build directory: {str(build)}")

    previous_dogfood_branch = parameters.previous_dogfood_branch
    current_shipping_branch = parameters.current_shipping_branch

    if not previous_dogfood_branch:
        previous_dogfood_branch = "release--2023-31"
    if not current_shipping_branch:
        current_shipping_branch = "release--2023.3"

    previous_dogfood_branch = previous_dogfood_branch.replace("/", "--")
    current_shipping_branch = current_shipping_branch.replace("/", "--")

    test_inputs = dict(
        test_scripts=build / "test-scripts",
        repo=build / "sdds3-repo",
        launchdarkly=build / "sdds3-launchdarkly",
        thin_installer=build / "test-thininstaller",
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
    test_inputs = {
        "x64": get_inputs(context, build, parameters)
    }
    machines = get_test_machines(test_inputs, parameters, x64_only=True, system_tests=True)

    parameters_json = json.dumps(parameters)

    for template_name, machine in machines:
        stage.task(task_name=template_name, func=robot_task, machine=machine, parameters_json=parameters_json)
    return


def sdds(stage: tap.Root, context: tap.PipelineContext, parameters: tap.Parameters):
    run_tests = parameters.run_system_tests != "false"

    with stage.parallel("sdds"):
        branch = context.branch

        is_release_branch = branch.startswith("release-") or branch.startswith("hotfix-")

        if is_release_branch:
            do_dev = False
            do_prod = True
        else:
            do_dev = not parameters.sdds_selection or parameters.sdds_selection == "dev"
            do_prod = parameters.sdds_selection == "prod"

        build = None
        if do_prod:
            build_sdds3_warehouse(stage=stage, mode="prod")
        if do_dev:
            build = build_sdds3_warehouse(stage=stage, mode="dev")
            build_sdds3_warehouse(stage=stage, mode="999")

    with stage.parallel("system_testing"):
        if run_tests and build:
            run_tap_tests(stage, context, parameters, build)

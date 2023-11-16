# Copyright 2023 Sophos Limited. All rights reserved.

import os
import json
from typing import Dict

import tap.v1 as tap
from tap._backend import Input
from tap._pipeline.tasks import ArtisanInput

from pipeline.common import get_test_machines, pip_install, get_os_packages, get_robot_args, python, X86_64, ARM64

SYSTEM_TEST_TIMEOUT = 9000
SYSTEM_TEST_TASK_TIMEOUT = 150


def build_sdds3_warehouse(stage: tap.Root, mode="dev", image="ubuntu2004_x64_bazel_20231109"):
    component = tap.Component(name="sdds3-warehouse-" + mode, base_version="1.0.0")
    return stage.artisan_build(name=mode,
                               component=component,
                               image=image,
                               release_package="./sdds/build/dev.xml",
                               mode=mode)


def get_inputs(context: tap.PipelineContext, sdds_build: ArtisanInput, sdds_build999: ArtisanInput, bazel_builds,
               parameters: tap.Parameters, build: str) -> Dict[str, Input]:
    previous_dogfood_branch = parameters.previous_dogfood_branch
    current_shipping_branch = parameters.current_shipping_branch

    if not previous_dogfood_branch:
        previous_dogfood_branch = "release--2023-40"
    if not current_shipping_branch:
        current_shipping_branch = "release--2023.3"

    previous_dogfood_branch = previous_dogfood_branch.replace("/", "--")
    current_shipping_branch = current_shipping_branch.replace("/", "--")

    if build == "linux_x64_rel":
        bazel_build = bazel_builds.get(X86_64)
    elif build == "linux_arm64_rel":
        bazel_build = bazel_builds.get(ARM64)
    else:
        return {}
    bazel_build_x64 = bazel_builds.get(X86_64)

    test_inputs = dict(
        test_scripts=context.artifact.from_folder('./sdds/TA'),
        repo=sdds_build / "sdds3-repo",
        launchdarkly=sdds_build / "sdds3-launchdarkly",
        static_suites=sdds_build / "prod-sdds3-static-suites",
        thin_installer=bazel_build_x64 / "thininstaller/thininstaller",
        base=bazel_build / f"base/{build}/installer",
        av=bazel_build / f"av/{build}/installer",
        edr=bazel_build / f"edr/{build}/installer",
        ej=bazel_build / f"eventjournaler/{build}/installer",
        lr=bazel_build / f"liveterminal/{build}/installer",
        ra=bazel_build / f"response_actions/{build}/installer",
        di=bazel_build / f"deviceisolation/{build}/installer",
        base_sdds_scripts=context.artifact.from_folder('./base/products/distribution'),
        common_test_libs=context.artifact.from_folder('./common/TA/libs'),
        common_test_robot=context.artifact.from_folder('./common/TA/robot'),
        SupportFiles=context.artifact.from_folder('./base/testUtils/SupportFiles'),
        rtd_content_rules=context.artifact.from_component("linuxep.runtimedetections", "develop", None, org="",
                                                          storage="esg-build-tested") / "build/sspl-rdrules",
        websocket_server=context.artifact.from_component("winep.liveterminal", "develop", None, org="",
                                                         storage="esg-build-tested") / "build/sspl-liveterminal/test-scripts",
        local_rep=context.artifact.from_component('ssplav-localrep', "released", None) / 'reputation',
        vdl=context.artifact.from_component('ssplav-dataseta', "released", None) / 'dataseta',
        ej_manual_tools=context.artifact.from_component("em.esg", "develop", None, org="",
                                                        storage="esg-build-tested") / f"build/common/journal/{build}/tools",
        liveterminal_test_scripts=context.artifact.from_component("winep.liveterminal", "develop", None, org="",
                                                                  storage="esg-build-tested") / "build/sspl-liveterminal/test-scripts",
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
        safestore_tools=context.artifact.from_component("em.esg", "develop", None, org="",
                                                        storage="esg-build-tested") / f"build/common/safestore/{build}/safestore",
        sdds3=context.artifact.from_component("em.esg", "develop", None, org="",
                                              storage="esg-build-tested") / f"build/sophlib/{build}/sdds3_tools",
    )

    arch = build.split("_")[1]
    if arch == "x64":
        test_inputs["rtd"] = context.artifact.from_component("linuxep.runtimedetections", "develop", None, org="",
                                                             storage="esg-build-tested") / "build/SDDS-COMPONENT"
        test_inputs["ml_model"] = context.artifact.from_component("ssplav-mlmodel3-x86_64", "released") / "model"
    elif arch == "arm64":
        test_inputs["rtd"] = context.artifact.from_component("linuxep.runtimedetections", "develop", None, org="",
                                                             storage="esg-build-tested") / "build/SDDS-COMPONENT-arm64"
        test_inputs["ml_model"] = context.artifact.from_component("ssplav-mlmodel3-arm64", "released") / "model"

    if sdds_build999:
        test_inputs["repo999"] = sdds_build999 / "sdds3-repo-999"
        test_inputs["launchdarkly999"] = sdds_build999 / "sdds3-launchdarkly-999"
        test_inputs["static_suites999"] = sdds_build999 / "prod-sdds3-static-suites-999"
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

    # TODO LINUXDAR-5855 remove this section when we stop signing with sha1
    try:
        distro = machine.template.split("_")[0]
        if distro in ["centos9stream", "rhel91"]:
            machine.run("update-crypto-policies", "--set", "LEGACY", timeout=30)
    except Exception as ex:
        print(f"On updating openssl policy: {ex}")

    try:
        pip_install(machine, "-r", machine.inputs.test_scripts / "requirements.txt")

        os_packages = get_os_packages(machine)
        install_command = ["bash", machine.inputs.SupportFiles / "install_os_packages.sh"] + os_packages
        machine.run(*install_command)
    except Exception as ex:
        # the previous command will fail if user already exists. But this is not an error
        print(f"On adding installing requirements: {ex}")


@tap.timeout(task_timeout=SYSTEM_TEST_TASK_TIMEOUT)
def robot_task(machine: tap.Machine, parameters_json: str, robot_args: str, include_tag: str):
    print(f"test scripts: {machine.inputs.test_scripts}")
    print(f"robot_args: {robot_args}")
    print(f"include_tag: {include_tag}")
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

        platform, arch = machine.template.split("_")[:2]
        robot_exclusion_tags = ["MANUAL", "FAIL", "DISABLED", f"EXCLUDE_{platform.upper()}", f"EXCLUDE_{arch.upper()}",
                                f"EXCLUDE_{platform.upper()}_{arch.upper()}"]
        if "CENTRAL_API_CLIENT_ID" not in environment or "CENTRAL_API_CLIENT_SECRET" not in environment:
            robot_exclusion_tags.append("FIXED_VERSIONS")

        if arch == "arm64":
            if platform in ["amzlinux2", "centos8stream", "debian10", "rhel87"]:
                # These ARM64 platforms have too old of a kernel to work with ARM64 RTD, so don't run tests which check
                # RTD logs for errors
                robot_exclusion_tags.append("RTD_CHECKED")

        install_requirements(machine)
        if include_tag:
            machine.run(*robot_args.split(), 'python3', machine.inputs.test_scripts / 'bin/RobotFramework.py',
                        '--include', include_tag,
                        '--exclude', *robot_exclusion_tags,
                        environment=environment,
                        timeout=SYSTEM_TEST_TIMEOUT)
        else:
            machine.run(*robot_args.split(),
                        python(machine),
                        machine.inputs.test_scripts / 'bin/RobotFramework.py',
                        '--exclude', *robot_exclusion_tags,
                        environment=environment,
                        timeout=SYSTEM_TEST_TIMEOUT)
    finally:
        machine.run(python(machine), machine.inputs.test_scripts / "bin/move_robot_results.py")
        machine.output_artifact("/opt/test/logs", "logs")
        machine.output_artifact("/opt/test/results", "results")


def run_tap_tests(stage: tap.Root, context: tap.PipelineContext, parameters: tap.Parameters, build, build999,
                  bazel_builds):
    default_include_tags = "TAP_PARALLEL1,TAP_PARALLEL2,TAP_PARALLEL3,TAP_PARALLEL4"
    test_inputs = {
        "x64": get_inputs(context, build, build999, bazel_builds, parameters, "linux_x64_rel"),
        "arm64": get_inputs(context, build, build999, bazel_builds, parameters, "linux_arm64_rel")
    }
    test_machines = get_test_machines(test_inputs, parameters, system_tests=True)
    robot_args = get_robot_args(parameters)

    parameters_json = json.dumps(parameters)

    if robot_args:
        for template_name, machine in test_machines:
            stage.task(task_name=template_name,
                       func=robot_task,
                       machine=machine,
                       parameters_json=parameters_json,
                       robot_args=robot_args,
                       include_tag="")
    else:
        includedtags = parameters.include_tags or default_include_tags
        for include in includedtags.split(","):
            with stage.parallel(include):
                for template_name, machine in test_machines:
                    stage.task(task_name=template_name,
                               func=robot_task,
                               machine=machine,
                               parameters_json=parameters_json,
                               robot_args="",
                               include_tag=include)
    return


def sdds(stage: tap.Root, context: tap.PipelineContext, parameters: tap.Parameters, run_system_tests, bazel_builds):
    build = None
    build999 = None

    with stage.parallel("sdds"):
        branch = context.branch
        is_release_branch = branch.startswith("release-") or branch.startswith("hotfix-")

        do_prod = False
        do_dev = False

        if is_release_branch or parameters.sdds_options == "build_prod":
            do_prod = True
        else:
            do_dev = True

        if do_prod:
            build_sdds3_warehouse(stage=stage, mode="prod")
        if do_dev:
            build = build_sdds3_warehouse(stage=stage, mode="dev")
            build999 = build_sdds3_warehouse(stage=stage, mode="999")

    if run_system_tests and build:
        with stage.parallel("system_testing"):
            run_tap_tests(stage, context, parameters, build, build999, bazel_builds)

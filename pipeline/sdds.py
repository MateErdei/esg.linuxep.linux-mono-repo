# Copyright 2023 Sophos Limited. All rights reserved.

import json
from typing import Any, Dict, List

import tap.v1 as tap
from tap._backend import Input
from tap._pipeline.tasks import ArtisanInput
from dotdict import DotDict

from pipeline.common import (
    get_test_machines,
    get_robot_args,
    X86_64,
    SDDS,
    SDDS999,
    get_test_builds,
    get_build_output_for_arch,
    stage_task,
    stage_tap_machine,
    run_robot_tests,
)

SYSTEM_TEST_TIMEOUT = 9000
SYSTEM_TEST_TASK_TIMEOUT = 150


def do_prod_or_dev_based_on_branch(context: tap.PipelineContext, parameters: tap.Parameters):
    branch = context.branch
    is_release_branch = branch.startswith("release-") or branch.startswith("hotfix-")

    do_prod = False
    do_dev = False

    if (is_release_branch or parameters.sdds_options == "build_prod_no_system_tests"
            or parameters.sdds_options == "build_prod_system_tests"):
        do_prod = True
    else:
        do_dev = True

    return {"do_dev": do_dev, "do_prod": do_prod}


def build_sdds3_warehouse(stage: tap.Root, mode="dev", image="ubuntu2004_x64_bazel_20231109"):
    component = tap.Component(name="sdds3-warehouse-" + mode, base_version="1.0.0")
    return stage.artisan_build(
        name=mode, component=component, image=image, release_package="./sdds/build/dev.xml", mode=mode
    )


def get_inputs(
    context: tap.PipelineContext,
    sdds_build: ArtisanInput,
    sdds_build999: ArtisanInput,
    build_output: ArtisanInput,
    x86_64_build_output: ArtisanInput,
    parameters: tap.Parameters,
    build: str,
) -> Dict[str, Input]:
    previous_dogfood_branch = parameters.previous_dogfood_branch
    previous_dogfood_branch_build_instance = parameters.previous_dogfood_branch_build_instance
    current_shipping_branch = parameters.current_shipping_branch
    current_shipping_branch_build_instance = parameters.current_shipping_branch_build_instance

    if not previous_dogfood_branch:
        previous_dogfood_branch = "release--2023.4"
        previous_dogfood_branch_build_instance = "20231117142146-704d26b9d2f10d7bbc843d378c6c700dcf500074-1rARjl"
    if not current_shipping_branch:
        current_shipping_branch = "release--2023.4"
        current_shipping_branch_build_instance = "20231117142146-704d26b9d2f10d7bbc843d378c6c700dcf500074-1rARjl"

    previous_dogfood_branch = previous_dogfood_branch.replace("/", "--")
    current_shipping_branch = current_shipping_branch.replace("/", "--")

    arch = build.split("_")[1]

    test_inputs = dict(
        test_scripts=context.artifact.from_folder("./sdds/TA"),
        static_suites=sdds_build / "prod-sdds3-static-suites",
        thin_installer=x86_64_build_output / "thininstaller/thininstaller",
        base=build_output / f"base/{build}/installer",
        av=build_output / f"av/{build}/installer",
        edr=build_output / f"edr/{build}/installer",
        ej=build_output / f"eventjournaler/{build}/installer",
        lr=build_output / f"liveterminal/{build}/installer",
        ra=build_output / f"response_actions/{build}/installer",
        di=build_output / f"deviceisolation/{build}/installer",
        base_sdds_scripts=context.artifact.from_folder("./base/products/distribution"),
        common_test_libs=context.artifact.from_folder("./common/TA/libs"),
        common_test_utils=context.artifact.from_folder("./common/TA/utils"),
        common_test_robot=context.artifact.from_folder("./common/TA/robot"),
        sspl_rdrules_scit=context.artifact.from_component(
            "linuxep.runtimedetections", "develop", None, org="", storage="esg-build-tested"
        )
        / "build/sspl-rdrules-scit",
        SupportFiles=context.artifact.from_folder("./base/testUtils/SupportFiles"),
        rtd_content_rules=context.artifact.from_component(
            "linuxep.runtimedetections", "develop", None, org="", storage="esg-build-tested"
        )
        / "build/sspl-rdrules",
        websocket_server=context.artifact.from_component(
            "winep.liveterminal", "develop", None, org="", storage="esg-build-tested"
        )
        / "build/sspl-liveterminal/test-scripts",
        ej_manual_tools=context.artifact.from_component("em.esg", "develop", None, org="", storage="esg-build-tested")
        / f"build/common/journal/linux_{arch}_rel/tools",
        liveterminal_test_scripts=context.artifact.from_component(
            "winep.liveterminal", "develop", None, org="", storage="esg-build-tested"
        )
        / "build/sspl-liveterminal/test-scripts",
        dogfood_launch_darkly=context.artifact.from_component(
            "linuxep.linux-mono-repo", previous_dogfood_branch, previous_dogfood_branch_build_instance, org="", storage="esg-build-candidate"
        )
        / "build/prod-sdds3-launchdarkly",
        dogfood_repo=context.artifact.from_component(
            "linuxep.linux-mono-repo", previous_dogfood_branch, previous_dogfood_branch_build_instance, org="", storage="esg-build-candidate"
        )
        / "build/prod-sdds3-repo",
        current_shipping_launch_darkly=context.artifact.from_component(
            "linuxep.linux-mono-repo", current_shipping_branch, current_shipping_branch_build_instance, org="", storage="esg-build-candidate"
        )
        / "build/prod-sdds3-launchdarkly",
        current_shipping_repo=context.artifact.from_component(
            "linuxep.linux-mono-repo", current_shipping_branch, current_shipping_branch_build_instance, org="", storage="esg-build-candidate"
        )
        / "build/prod-sdds3-repo",
        safestore_tools=context.artifact.from_component("em.esg", "develop", None, org="", storage="esg-build-tested")
        / f"build/common/safestore/linux_{arch}_rel/safestore",
        sdds3=context.artifact.from_component("em.esg", "develop", None, org="", storage="esg-build-tested")
        / f"build/sophlib/linux_{arch}_rel/sdds3_tools",
    )

    do_prod_dev_dict = do_prod_or_dev_based_on_branch(context, parameters)
    do_dev, do_prod = do_prod_dev_dict["do_dev"], do_prod_dev_dict["do_prod"]

    if do_prod:
        test_inputs["dev_sdds3"] = context.artifact.from_component("linuxep.linux-mono-repo", "develop", None, org="",
                                                                   storage="esg-build-tested") / "build/sdds3-repo"
        test_inputs["repo"] = sdds_build / "prod-sdds3-repo"
        test_inputs["launchdarkly"] = sdds_build / "prod-sdds3-launchdarkly"
    elif do_dev:
        test_inputs["repo"] = sdds_build / "sdds3-repo"
        test_inputs["launchdarkly"] = sdds_build / "sdds3-launchdarkly"

    arch = build.split("_")[1]
    if arch == "x64":
        test_inputs["rtd"] = (
            context.artifact.from_component(
                "linuxep.runtimedetections", "develop", None, org="", storage="esg-build-tested"
            )
            / "build/SDDS-COMPONENT"
        )
    elif arch == "arm64":
        test_inputs["rtd"] = (
            context.artifact.from_component(
                "linuxep.runtimedetections", "develop", None, org="", storage="esg-build-tested"
            )
            / "build/SDDS-COMPONENT-arm64"
        )

    if sdds_build999:
        test_inputs["repo999"] = sdds_build999 / "sdds3-repo-999"
        test_inputs["launchdarkly999"] = sdds_build999 / "sdds3-launchdarkly-999"
        test_inputs["static_suites999"] = sdds_build999 / "prod-sdds3-static-suites-999"
    return test_inputs


@tap.timeout(task_timeout=SYSTEM_TEST_TASK_TIMEOUT)
def run_sdds_tests(machine: tap.Machine, robot_args_json: str):
    robot_exclusion_tags = []
    platform, arch = machine.template.split("_")[:2]
    if arch == "arm64" and platform in ["amzlinux2", "centos8stream", "debian10", "rhel87"]:
        # These ARM64 platforms have too old of a kernel to work with ARM64 RTD, so don't run tests which check
        # RTD logs for errors
        robot_exclusion_tags.append("RTD_CHECKED")

    extra_robot_args = []
    if len(robot_exclusion_tags) > 0:
        extra_robot_args += ["--exclude"] + robot_exclusion_tags

    run_robot_tests(
        machine, timeout_seconds=SYSTEM_TEST_TIMEOUT, robot_args=json.loads(robot_args_json) + extra_robot_args
    )


def stage_sdds_build(
    stage: tap.Root, context: tap.PipelineContext, parameters: tap.Parameters, outputs: Dict[str, Any]
):
    with stage.parallel("sdds"):
        do_prod_dev_dict = do_prod_or_dev_based_on_branch(context, parameters)
        do_dev, do_prod = do_prod_dev_dict["do_dev"], do_prod_dev_dict["do_prod"]

        if do_prod:
            outputs["sdds"] = build_sdds3_warehouse(stage=stage, mode="prod")
            outputs["sdds999"] = build_sdds3_warehouse(stage=stage, mode="999")
        if do_dev:
            outputs["sdds"] = build_sdds3_warehouse(stage=stage, mode="dev")
            outputs["sdds999"] = build_sdds3_warehouse(stage=stage, mode="999")


def stage_sdds_tests(
    stage: tap.Root,
    context: tap.PipelineContext,
    component: tap.Component,  # pylint: disable=unused-argument
    parameters: DotDict,
    outputs: Dict[str, Any],
    coverage_tasks: List[str],
):
    group_name = "system_tests"

    default_include_tags = "TAP_PARALLEL1,TAP_PARALLEL2"

    fixed_versions_x86 = []
    fixed_versions_arm64 = []
    for fts in context.releases.fixed_term_releases:
        if fts.device_class == 'LINUXEP':
            print(f"FTS name: {fts.name}, expiry: {fts.eol_date}, token: {fts.token}")
            # FTS 2023.3 is not available in Linux Mono-repo of with ARM64 build
            # Also do not test special FTS releases (ie. those with JIRA ticket ID in their name)
            if fts.name != "FTS 2023.3.0.23" and "LINUXDAR" not in fts.name:
                fixed_versions_x86.append(fts.name)
                fixed_versions_arm64.append(fts.name)

    with stage.parallel(group_name):
        for build in get_test_builds():
            build_output = get_build_output_for_arch(outputs, build)
            if not build_output:
                continue
            if X86_64 not in outputs or SDDS not in outputs or SDDS999 not in outputs:
                continue
            inputs = get_inputs(
                context, outputs[SDDS], outputs[SDDS999], build_output, outputs[X86_64], parameters, build
            )
            test_machines = get_test_machines(build, parameters)

            robot_args = get_robot_args(parameters)

            if parameters.get("central_api_client_id"):
                robot_args += ["--central-api-client-id", parameters["central_api_client_id"]]
            if parameters.get("central_api_client_secret"):
                robot_args += ["--central-api-client-secret", parameters["central_api_client_secret"]]


            includedtags = parameters.include_tags or default_include_tags

            for include in includedtags.split(","):
                for machine in test_machines:
                    fixed_version_array = []
                    if "arm" in machine:
                        if fixed_versions_arm64:
                            fixed_version_array.append("--fixed-versions")
                            for fixed_version in fixed_versions_arm64:
                                fixed_version_array.append(fixed_version)
                    else:
                        if fixed_versions_x86:
                            fixed_version_array.append("--fixed-versions")
                            for fixed_version in fixed_versions_x86:
                                fixed_version_array.append(fixed_version)
                    robot_args_json = json.dumps(robot_args + fixed_version_array +["--include", include])

                    stage_task(
                        stage=stage,
                        coverage_tasks=coverage_tasks,
                        group_name=group_name,
                        task_name=f"{include}_{build}_{machine}",
                        build=build,
                        func=run_sdds_tests,
                        machine=stage_tap_machine(machine, inputs, outputs, build),
                        robot_args_json=robot_args_json,
                    )

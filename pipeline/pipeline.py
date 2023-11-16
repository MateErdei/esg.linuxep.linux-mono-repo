# Copyright 2022-2023 Sophos Limited. All rights reserved.
import os.path

import tap.v1 as tap

from pipeline.av import stage_av_tests
from pipeline.base import stage_base_tests
from pipeline.common import get_package_version, truthy, is_coverage_enabled, stage_coverage_results_task
from pipeline.edr import stage_edr_tests
from pipeline.eventjournaler import stage_ej_tests
from pipeline.deviceisolation import stage_di_tests
from pipeline.liveterminal import stage_liveterminal_tests
from pipeline.sdds import stage_sdds_build, stage_sdds_tests
from pipeline import common

PACKAGE_PATH = "build/release-package.xml"
BUILD_TEMPLATE = 'centos79_x64_build_20230202'
BUILD_TEMPLATE_BAZEL = 'ubuntu2004_x64_bazel_20231109'

PACKAGE_PATH_BASE = "./base/build/release-package.xml"
PACKAGE_PATH_EDR = "./edr/build-files/release-package.xml"
PACKAGE_PATH_AV = "./av/build-files/release-package.xml"
PACKAGE_PATH_LIVETERMINAL = "./liveterminal/linux-release-package.xml"
PACKAGE_PATH_EJ = "./eventjournaler/build-files/release-package.xml"
PACKAGE_PATH_DI = "./deviceisolation/build-files/release-package.xml"
PACKAGE_PATH_SDDS = "./sdds/build/dev.xml"

BUILD_SELECTION_ALL = "all"
BUILD_SELECTION_BASE = "base"
BUILD_SELECTION_AV = "av"
BUILD_SELECTION_LIVETERMINAL = "liveterminal"
BUILD_SELECTION_EDR = "edr"
BUILD_SELECTION_EJ = "ej"
BUILD_SELECTION_DI = "di"


def builds_unified(stage: tap.Root, component, build_type: str, build_modes: dict):
    print("Build architectures unified")
    modes = build_modes.get(common.X86_64, []) + build_modes.get(common.arm64, [])
    if len(modes) > 0:
        build = stage.artisan_build(name=f"linux_{build_type}",
                                    component=component,
                                    image=BUILD_TEMPLATE_BAZEL,
                                    mode=",".join(modes),
                                    release_package=PACKAGE_PATH)
        build.arch = "unified"
        build.build_type = build_type
    else:
        build = None

    return {
        common.X86_64: build,
        common.arm64: build
    }


def builds_separate(stage: tap.Root, component, build_type: str, build_modes: dict):
    print("Build architectures separately")
    builds = {
        common.X86_64: None,
        common.arm64: None
    }
    for arch, modes in build_modes.items():
        if modes and len(modes) > 0:
            builds[arch] = stage.artisan_build(name=f"linux_{build_type}_{arch}",
                                               component=component,
                                               image=BUILD_TEMPLATE_BAZEL,
                                               mode=",".join(modes),
                                               release_package=PACKAGE_PATH)
            builds[arch].arch = arch
            builds[arch].build_type = build_type
    return builds


def do_builds(stage: tap.Root, component, parameters: tap.Parameters, build_type: str, build_modes: dict):
    if common.do_unified_build(parameters):
        return builds_unified(stage, component, build_type, build_modes)
    else:
        return builds_separate(stage, component, build_type, build_modes)


def bazel_pipeline(stage: tap.Root, context: tap.PipelineContext, parameters: tap.Parameters, test_selection: str, system_tests: bool):
    component = tap.Component(name="linux-mono-repo", base_version="1.0.0")

    rel_build = {}
    dbg_build = {}
    if common.x86_64_enabled(parameters):
        print("x86_64 build enabled")
        rel_build[common.X86_64] = ["san_lsnty", "all_lx64r"]
        dbg_build[common.X86_64] = ["all_lx64d"]
    if common.arm64_enabled(parameters):
        print("arm64 build enabled")
        rel_build[common.arm64] = ["all_la64r"]
        dbg_build[common.arm64] = ["all_la64d"]

    builds = do_builds(stage, component, parameters, "rel", rel_build)

    # Default to true here so that local builds work.
    linux_dbg = truthy(parameters.build_debug_bazel, "build_debug_bazel", True)
    print(f"parameters.build_debug_bazel = {parameters.build_debug_bazel}; Defaulting to linux_debug = {linux_dbg}")

    if linux_dbg:
        do_builds(stage, component, parameters, "dbg", dbg_build)

    if is_coverage_enabled(parameters):
        builds["bullseye"] = stage.artisan_build(name=f"linux_coverage",
                                                 component=component,
                                                 image=BUILD_TEMPLATE_BAZEL,
                                                 mode="all_lcov",
                                                 release_package=PACKAGE_PATH)

    if parameters.sdds_options != "build_none":
        stage_sdds_build(stage, context, parameters, builds)

    if parameters.test_platform != "run_none":
        with stage.parallel('testing'):
            coverage_tasks = []

            if test_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_AV]:
                stage_av_tests(stage, context, component, parameters, builds, coverage_tasks)

            if test_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_BASE]:
                stage_base_tests(stage, context, component, parameters, builds, coverage_tasks)

            if test_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_EJ]:
                stage_ej_tests(stage, context, component, parameters, builds, coverage_tasks)

            if test_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_DI]:
                stage_di_tests(stage, context, component, parameters, builds, coverage_tasks)

            if test_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_EDR]:
                stage_edr_tests(stage, context, component, parameters, builds, coverage_tasks)

            if test_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_LIVETERMINAL]:
                stage_liveterminal_tests(stage, context, component, parameters, builds, coverage_tasks)

            if system_tests:
                stage_sdds_tests(stage, context, component, parameters, builds, coverage_tasks)

            stage_coverage_results_task(stage, context, component, parameters, builds, coverage_tasks)


@tap.pipeline(version=1, component='linux-mono-repo')
def linux_mono_repo(stage: tap.Root, context: tap.PipelineContext, parameters: tap.Parameters):
    os.environ["BRANCH_NAME"] = context.branch
    cmake = truthy(parameters.build_with_cmake, "build_with_cmake", True)
    system_tests = (not parameters.sdds_options or parameters.sdds_options == "build_dev_system_tests")
    print(f"parameters.build_with_cmake = {parameters.build_with_cmake}; Defaulting to cmake = {cmake}")
    print(f"parameters.sdds_options = {parameters.sdds_options}; Defaulting to system_tests = {system_tests}")
    print(f"Coverage = {is_coverage_enabled(parameters)} (parameters.mode = {parameters.mode}, parameters.code_coverage = {parameters.code_coverage})")

    test_selection = parameters.test_selection or BUILD_SELECTION_ALL

    with stage.parallel("products"):
        bazel_pipeline(stage, context, parameters, test_selection, system_tests)

        if cmake:
            with stage.parallel("cmake"):
                cmake_pipeline(stage, context, test_selection)


def cmake_pipeline(stage: tap.Root, context: tap.PipelineContext, test_selection: str):
    assert (os.path.exists(PACKAGE_PATH_BASE))
    assert (os.path.exists(PACKAGE_PATH_EDR))
    assert (os.path.exists(PACKAGE_PATH_AV))
    assert (os.path.exists(PACKAGE_PATH_LIVETERMINAL))
    assert (os.path.exists(PACKAGE_PATH_EJ))
    print("Running cmake pipeline")
    running_in_ci = "CI" in os.environ and os.environ["CI"] == "true"

    base_component = tap.Component(name="sspl_base", base_version=get_package_version(PACKAGE_PATH_BASE))
    edr_component = tap.Component(name="edr", base_version=get_package_version(PACKAGE_PATH_EDR))
    ej_component = tap.Component(name='sspl-event-journaler-plugin', base_version=get_package_version(PACKAGE_PATH_EJ))
    av_component = tap.Component(name='sspl-plugin-anti-virus', base_version=get_package_version(PACKAGE_PATH_AV))
    liveterminal_component = tap.Component(name='liveterminal_linux',
                                           base_version=get_package_version(PACKAGE_PATH_LIVETERMINAL))

    if running_in_ci:
        # Base always builds
        # Pluginapi needed for fuzzers and RTD
        stage.artisan_build(name="base_release",
                            component=base_component,
                            image=BUILD_TEMPLATE,
                            mode="release",
                            release_package=PACKAGE_PATH_BASE)

        stage.artisan_build(name="base_analysis",
                            component=base_component,
                            image=BUILD_TEMPLATE,
                            mode="analysis",
                            release_package=PACKAGE_PATH_BASE)

        if test_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_EDR]:
            stage.artisan_build(name="edr_analysis",
                                component=edr_component,
                                image=BUILD_TEMPLATE,
                                mode="analysis",
                                release_package=PACKAGE_PATH_EDR)

        if test_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_EJ]:
            stage.artisan_build(name="ej_analysis",
                                component=ej_component,
                                image=BUILD_TEMPLATE,
                                mode="analysis",
                                release_package=PACKAGE_PATH_EJ)

        if test_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_AV]:
            stage.artisan_build(name="av_analysis",
                                component=av_component,
                                image=BUILD_TEMPLATE,
                                mode="analysis",
                                release_package=PACKAGE_PATH_AV)

        if test_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_LIVETERMINAL]:
            stage.artisan_build(name="liveterminal_analysis",
                                component=liveterminal_component,
                                image=BUILD_TEMPLATE,
                                mode="analysis",
                                release_package=PACKAGE_PATH_LIVETERMINAL)

# Copyright 2022-2023 Sophos Limited. All rights reserved.
import os.path

import tap.v1 as tap

from pipeline import base
from pipeline.av import run_av_tests, run_av_coverage_tests
from pipeline.base import run_base_tests, run_base_coverage_tests

from pipeline.common import get_package_version, \
    INDEPENDENT_MODE, RELEASE_MODE, ANALYSIS_MODE, NINE_NINE_NINE_MODE, \
    ZERO_SIX_ZERO_MODE, COVERAGE_MODE, truthy
from pipeline.edr import run_edr_tests, run_edr_coverage_tests
from pipeline.eventjournaler import run_ej_tests, run_ej_coverage_tests
from pipeline.liveterminal import run_liveterminal_tests, run_liveterminal_coverage_tests
from pipeline.sdds import sdds
from pipeline import common

PACKAGE_PATH = "build/release-package.xml"
BUILD_TEMPLATE = 'centos79_x64_build_20230202'
BUILD_TEMPLATE_BAZEL = 'centos79_x64_bazel_20230808'

PACKAGE_PATH_EDR = "./edr/build-files/release-package.xml"
PACKAGE_PATH_AV = "./av/build-files/release-package.xml"
PACKAGE_PATH_LIVETERMINAL = "./liveterminal/linux-release-package.xml"
PACKAGE_PATH_EJ = "./eventjournaler/build-files/release-package.xml"
PACKAGE_PATH_SDDS = "./sdds/build/dev.xml"

BUILD_SELECTION_ALL = "all"
BUILD_SELECTION_BASE = "base"
BUILD_SELECTION_AV = "av"
BUILD_SELECTION_LIVETERMINAL = "liveterminal"
BUILD_SELECTION_EDR = "edr"
BUILD_SELECTION_EJ = "ej"


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


def bazel_pipeline(stage: tap.Root, context: tap.PipelineContext, parameters: tap.Parameters, mode: str
                   , test_selection: str):
    print("Running bazel pipeline")
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

    if parameters.test_platform != "run_none":
        with stage.parallel('testing'):
            if mode == RELEASE_MODE:
                if test_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_AV]:
                    run_av_tests(stage, context, builds, mode, parameters, bazel=True)

                if test_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_BASE]:
                    run_base_tests(stage, context, builds, mode, parameters)

                if test_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_EJ]:
                    run_ej_tests(stage, context, builds, mode, parameters)

                if test_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_EDR]:
                    run_edr_tests(stage, context, builds, mode, parameters)

                if test_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_LIVETERMINAL]:
                    run_liveterminal_tests(stage, context, builds, mode, parameters)
    return builds


@tap.pipeline(version=1, component='linux-mono-repo')
def linux_mono_repo(stage: tap.Root, context: tap.PipelineContext, parameters: tap.Parameters):
    # In CI parameters.mode will be set
    mode = parameters.mode or RELEASE_MODE
    os.environ["BRANCH_NAME"] = context.branch

    cmake = truthy(parameters.build_with_cmake, "build_with_cmake", True)
    system_tests = (parameters.sdds_options == "build_dev_system_tests")
    print(f"parameters.mode = {parameters.mode}; Defaulting to mode = {mode}")
    print(f"parameters.build_with_cmake = {parameters.build_with_cmake}; Defaulting to cmake = {cmake}")
    print(f"parameters.sdds_options = {parameters.sdds_options}; Defaulting to system_tests = {system_tests}")
    test_selection = parameters.test_selection or BUILD_SELECTION_ALL

    bazel_builds = {}
    with stage.parallel("products"):
        with stage.parallel("bazel"):
            bazel_builds = bazel_pipeline(stage, context, parameters, mode, test_selection)

        if cmake:
            with stage.parallel("cmake"):
                cmake_pipeline(stage, context, parameters, mode, test_selection)

        if (
            test_selection == BUILD_SELECTION_ALL and
            parameters.sdds_options != "build_none" and
            parameters.architectures != "x86_64 only" and
            parameters.architectures != "arm64 only" and
            mode == RELEASE_MODE
        ):
            sdds(stage, context, parameters, system_tests, bazel_builds)


def cmake_pipeline(stage: tap.Root, context: tap.PipelineContext, parameters: tap.Parameters, mode: str,
                   test_selection: str):
    assert (os.path.exists(base.PACKAGE_PATH))
    assert (os.path.exists(PACKAGE_PATH_EDR))
    assert (os.path.exists(PACKAGE_PATH_AV))
    assert (os.path.exists(PACKAGE_PATH_LIVETERMINAL))
    assert (os.path.exists(PACKAGE_PATH_EJ))
    print("Running cmake pipeline")
    running_in_ci = "CI" in os.environ and os.environ["CI"] == "true"

    if parameters.include_tags is None:
        parameters.include_tags = ""

    # Base always builds
    base_component = tap.Component(name="sspl_base", base_version=get_package_version(base.PACKAGE_PATH))

    with stage.parallel('base'):
        if mode == RELEASE_MODE:
            base_build = stage.artisan_build(name=f"base_{RELEASE_MODE}",
                                             component=base_component,
                                             image=BUILD_TEMPLATE,
                                             mode=RELEASE_MODE,
                                             release_package=base.PACKAGE_PATH)

            if running_in_ci:
                base_analysis_build = stage.artisan_build(name=f"base_{ANALYSIS_MODE}",
                                                          component=base_component,
                                                          image=BUILD_TEMPLATE,
                                                          mode=ANALYSIS_MODE,
                                                          release_package=base.PACKAGE_PATH)

        elif mode == COVERAGE_MODE:
            base_coverage_build = stage.artisan_build(name=f"base_{COVERAGE_MODE}",
                                                      component=base_component,
                                                      image=BUILD_TEMPLATE,
                                                      mode=COVERAGE_MODE,
                                                      release_package=base.PACKAGE_PATH)

            # We also need a release build of Base so that we can use the plugin API in the coverage builds of plugins
            base_build = stage.artisan_build(name=f"base_{RELEASE_MODE}",
                                             component=base_component,
                                             image=BUILD_TEMPLATE,
                                             mode=RELEASE_MODE,
                                             release_package=base.PACKAGE_PATH)

    # Plugins
    edr_component = tap.Component(name="edr", base_version=get_package_version(PACKAGE_PATH_EDR))
    ej_component = tap.Component(name='sspl-event-journaler-plugin', base_version=get_package_version(PACKAGE_PATH_EJ))
    av_component = tap.Component(name='sspl-plugin-anti-virus', base_version=get_package_version(PACKAGE_PATH_AV))
    liveterminal_component = tap.Component(name='liveterminal_linux',
                                           base_version=get_package_version(PACKAGE_PATH_LIVETERMINAL))
    with stage.parallel('plugins'):
        if mode == RELEASE_MODE:
            # EDR
            if test_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_EDR]:
                if running_in_ci:
                    edr_analysis_build = stage.artisan_build(name=f"edr_{ANALYSIS_MODE}",
                                                             component=edr_component,
                                                             image=BUILD_TEMPLATE,
                                                             mode=ANALYSIS_MODE,
                                                             release_package=PACKAGE_PATH_EDR)

            # Event Journaler
            if test_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_EJ]:
                if running_in_ci:
                    ej_analysis_build = stage.artisan_build(name=f"ej_{ANALYSIS_MODE}",
                                                            component=ej_component,
                                                            image=BUILD_TEMPLATE,
                                                            mode=ANALYSIS_MODE,
                                                            release_package=PACKAGE_PATH_EJ)

            # AV
            if test_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_AV]:
                if running_in_ci:
                    av_analysis_build = stage.artisan_build(name=f"av_{ANALYSIS_MODE}",
                                                            component=av_component,
                                                            image=BUILD_TEMPLATE,
                                                            mode=ANALYSIS_MODE,
                                                            release_package=PACKAGE_PATH_AV)

        elif mode == COVERAGE_MODE:
            if test_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_EDR]:
                edr_coverage_build = stage.artisan_build(name=f"edr_{COVERAGE_MODE}",
                                                         component=edr_component,
                                                         image=BUILD_TEMPLATE,
                                                         mode=COVERAGE_MODE,
                                                         release_package=PACKAGE_PATH_EDR)

            if test_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_EJ]:
                ej_coverage_build = stage.artisan_build(name=f"ej_{COVERAGE_MODE}",
                                                        component=ej_component,
                                                        image=BUILD_TEMPLATE,
                                                        mode=COVERAGE_MODE,
                                                        release_package=PACKAGE_PATH_EJ)

            if test_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_AV]:
                av_coverage_build = stage.artisan_build(name=f"av_{COVERAGE_MODE}",
                                                        component=av_component,
                                                        image=BUILD_TEMPLATE,
                                                        mode=COVERAGE_MODE,
                                                        release_package=PACKAGE_PATH_AV)
            if test_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_LIVETERMINAL]:
                liveterminal_coverage_build = stage.artisan_build(name=f"liveterminal_{COVERAGE_MODE}",
                                                                  component=liveterminal_component,
                                                                  image=BUILD_TEMPLATE,
                                                                  mode=COVERAGE_MODE,
                                                                  release_package=PACKAGE_PATH_LIVETERMINAL)

    with stage.parallel('testing'):
        # run_tests can be None when tap runs locally, this needs to be "!= False" instead of "if parameters.run_tests:"
        if parameters.test_platform != "run_none" and mode == COVERAGE_MODE:
            if test_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_BASE]:
                run_base_coverage_tests(stage, context, base_coverage_build, mode, parameters)

            if test_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_EDR]:
                run_edr_coverage_tests(stage, context, edr_coverage_build, mode, parameters)

            if test_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_EJ]:
                run_ej_coverage_tests(stage, context, ej_coverage_build, mode, parameters)

            if test_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_AV]:
                run_av_coverage_tests(stage, context, av_coverage_build, mode, parameters)

            if test_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_LIVETERMINAL]:
                run_liveterminal_coverage_tests(stage, context, liveterminal_coverage_build, mode, parameters)

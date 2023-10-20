# Copyright 2022-2023 Sophos Limited. All rights reserved.
import os.path

import tap.v1 as tap

from pipeline import base
from pipeline.av import run_av_tests, run_av_coverage_tests
from pipeline.base import run_base_tests, run_base_coverage_tests

from pipeline.common import get_package_version, \
    INDEPENDENT_MODE, RELEASE_MODE, ANALYSIS_MODE, NINE_NINE_NINE_MODE, \
    ZERO_SIX_ZERO_MODE, COVERAGE_MODE
from pipeline.edr import run_edr_tests, run_edr_coverage_tests
from pipeline.eventjournaler import run_ej_tests, run_ej_coverage_tests
from pipeline.liveterminal import run_liveterminal_tests, run_liveterminal_coverage_tests
from pipeline.sdds import sdds


PACKAGE_PATH = "build/release-package.xml"
BUILD_TEMPLATE = 'centos79_x64_build_20230202'
BUILD_TEMPLATE_BAZEL = 'centos79_x64_bazel_20230512'

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


def bazel_pipeline(stage: tap.Root, context: tap.PipelineContext, parameters: tap.Parameters, mode: str, build_selection: str):
    print("Running bazel pipeline")
    component = tap.Component(name="linux-mono-repo", base_version="1.0.0")

    build = stage.artisan_build(name="linux_rel",
                                     component=component,
                                     image=BUILD_TEMPLATE_BAZEL,
                                     mode="san_lsnty,all_lx64r,all_la64r",
                                     release_package=PACKAGE_PATH)

    if parameters.build_debug_bazel != False:
        stage.artisan_build(name="linux_dbg",
                            component=component,
                            image=BUILD_TEMPLATE_BAZEL,
                            mode="all_lx64d,all_la64d",
                            release_package=PACKAGE_PATH)

    if parameters.run_tests != False:
        with stage.parallel('testing'):
            if mode == RELEASE_MODE:
                if build_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_AV]:
                    run_av_tests(stage, context, build, mode, parameters, bazel=True)

                if build_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_BASE]:
                    run_base_tests(stage, context, build, mode, parameters)

                if build_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_EJ]:
                    run_ej_tests(stage, context, build, mode, parameters)

                if build_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_LIVETERMINAL]:
                    run_liveterminal_tests(stage, context, build, mode, parameters)


def truthy(value, parameterName, defaultValue):
    if value in (True, "true", 1, "1", "T", "True"):
        return True
    if value in (False, "false", 0, "0", "F", "False"):
        return False
    if value is None:
        return defaultValue
    print(f"{parameterName} is not a good truthy value: {value}, assuming {defaultValue}")
    return defaultValue


@tap.pipeline(version=1, component='linux-mono-repo')
def linux_mono_repo(stage: tap.Root, context: tap.PipelineContext, parameters: tap.Parameters):
    # In CI parameters.mode will be set
    mode = parameters.mode or RELEASE_MODE
    cmake = truthy(parameters.build_with_cmake, "build_with_cmake", True)
    print(f"parameters.mode = {parameters.mode}; MODE = {mode}; cmake = {cmake} - {parameters.build_with_cmake}")

    build_selection = parameters.build_selection or BUILD_SELECTION_ALL

    with stage.parallel("products"):
        with stage.parallel("bazel"):
            bazel_pipeline(stage, context, parameters, mode, build_selection)

        if cmake:
            with stage.parallel("cmake"):
                cmake_pipeline(stage, context, parameters, mode, build_selection)

        if (
                build_selection == BUILD_SELECTION_ALL and
                parameters.run_system_tests != "false" and
                mode == RELEASE_MODE and
                cmake
        ):
            sdds(stage, context, parameters)


def cmake_pipeline(stage: tap.Root, context: tap.PipelineContext, parameters: tap.Parameters, mode: str, build_selection: str):
    assert (os.path.exists(base.PACKAGE_PATH))
    assert (os.path.exists(PACKAGE_PATH_EDR))
    assert (os.path.exists(PACKAGE_PATH_AV))
    assert (os.path.exists(PACKAGE_PATH_LIVETERMINAL))
    assert (os.path.exists(PACKAGE_PATH_EJ))
    print("Running cmake pipeline")
    os.environ["BRANCH_NAME"] = context.branch
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
    liveterminal_component = tap.Component(name='liveterminal_linux', base_version=get_package_version(PACKAGE_PATH_LIVETERMINAL))
    with stage.parallel('plugins'):
        if mode == INDEPENDENT_MODE:
            if build_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_EDR]:
                edr_build = stage.artisan_build(name=f"edr_{mode}",
                                                component=edr_component,
                                                image=BUILD_TEMPLATE,
                                                mode=mode,
                                                release_package=PACKAGE_PATH_EDR)
        elif mode == RELEASE_MODE:
            # EDR
            if build_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_EDR]:
                edr_build = stage.artisan_build(name=f"edr_{RELEASE_MODE}",
                                                component=edr_component,
                                                image=BUILD_TEMPLATE,
                                                mode=RELEASE_MODE,
                                                release_package=PACKAGE_PATH_EDR)
                if running_in_ci:
                    edr_analysis_build = stage.artisan_build(name=f"edr_{ANALYSIS_MODE}",
                                                             component=edr_component,
                                                             image=BUILD_TEMPLATE,
                                                             mode=ANALYSIS_MODE,
                                                             release_package=PACKAGE_PATH_EDR)

            # Event Journaler
            if build_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_EJ]:
                if running_in_ci:
                    ej_analysis_build = stage.artisan_build(name=f"ej_{ANALYSIS_MODE}",
                                                            component=ej_component,
                                                            image=BUILD_TEMPLATE,
                                                            mode=ANALYSIS_MODE,
                                                            release_package=PACKAGE_PATH_EJ)

            # AV
            if build_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_AV]:
                if running_in_ci:
                    av_analysis_build = stage.artisan_build(name=f"av_{ANALYSIS_MODE}",
                                                            component=av_component,
                                                            image=BUILD_TEMPLATE,
                                                            mode=ANALYSIS_MODE,
                                                            release_package=PACKAGE_PATH_AV)

        elif mode == COVERAGE_MODE:
            if build_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_EDR]:
                edr_coverage_build = stage.artisan_build(name=f"edr_{COVERAGE_MODE}",
                                                         component=edr_component,
                                                         image=BUILD_TEMPLATE,
                                                         mode=COVERAGE_MODE,
                                                         release_package=PACKAGE_PATH_EDR)

            if build_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_EJ]:
                ej_coverage_build = stage.artisan_build(name=f"ej_{COVERAGE_MODE}",
                                                        component=ej_component,
                                                        image=BUILD_TEMPLATE,
                                                        mode=COVERAGE_MODE,
                                                        release_package=PACKAGE_PATH_EJ)

            if build_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_AV]:
                av_coverage_build = stage.artisan_build(name=f"av_{COVERAGE_MODE}",
                                                        component=av_component,
                                                        image=BUILD_TEMPLATE,
                                                        mode=COVERAGE_MODE,
                                                        release_package=PACKAGE_PATH_AV)
            if build_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_LIVETERMINAL]:
                liveterminal_coverage_build = stage.artisan_build(name=f"liveterminal_{COVERAGE_MODE}",
                                                        component=liveterminal_component,
                                                        image=BUILD_TEMPLATE,
                                                        mode=COVERAGE_MODE,
                                                        release_package=PACKAGE_PATH_LIVETERMINAL)

    with stage.parallel('testing'):
        # run_tests can be None when tap runs locally, this needs to be "!= False" instead of "if parameters.run_tests:"
        if parameters.run_tests != False:
            if mode == RELEASE_MODE:
                if build_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_EDR]:
                    run_edr_tests(stage, context, edr_build, mode, parameters)

            elif mode == COVERAGE_MODE:
                if build_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_BASE]:
                    run_base_coverage_tests(stage, context, base_coverage_build, mode, parameters)
    
                if build_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_EDR]:
                    run_edr_coverage_tests(stage, context, edr_coverage_build, mode, parameters)
    
                if build_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_EJ]:
                    run_ej_coverage_tests(stage, context, ej_coverage_build, mode, parameters)
    
                if build_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_AV]:
                    run_av_coverage_tests(stage, context, av_coverage_build, mode, parameters)

                if build_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_LIVETERMINAL]:
                    run_liveterminal_coverage_tests(stage, context, liveterminal_coverage_build, mode, parameters)
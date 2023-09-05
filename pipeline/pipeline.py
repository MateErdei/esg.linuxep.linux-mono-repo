import tap.v1 as tap

from pipeline import base
from pipeline.av import run_av_tests, run_av_coverage_tests
from pipeline.base import run_base_tests, run_base_coverage_tests

from pipeline.common import get_package_version, RELEASE_MODE, ANALYSIS_MODE, NINE_NINE_NINE_MODE, \
    ZERO_SIX_ZERO_MODE, COVERAGE_MODE
from pipeline.edr import run_edr_tests, run_edr_coverage_tests
from pipeline.eventjournaler import run_ej_tests, run_ej_coverage_tests


PACKAGE_PATH = "build/release-package.xml"
BUILD_TEMPLATE = 'centos79_x64_build_20230202'
BUILD_TEMPLATE_BAZEL = 'centos79_x64_bazel_20230512'

PACKAGE_PATH_EDR = "./edr/build-files/release-package.xml"
PACKAGE_PATH_AV = "./av/build-files/release-package.xml"
PACKAGE_PATH_EJ = "./eventjournaler/build-files/release-package.xml"

BUILD_SELECTION_ALL = "all"
BUILD_SELECTION_BASE = "base"
BUILD_SELECTION_AV = "av"
BUILD_SELECTION_EDR = "edr"
BUILD_SELECTION_EJ = "ej"


@tap.pipeline(version=1, component='linux-mono-repo')
def linux_mono_repo(stage: tap.Root, context: tap.PipelineContext, parameters: tap.Parameters):
    assert (os.path.exists(base.PACKAGE_PATH))
    assert (os.path.exists(PACKAGE_PATH_EDR))
    assert (os.path.exists(PACKAGE_PATH_AV))
    assert (os.path.exists(PACKAGE_PATH_EJ))

    running_in_ci = "CI" in os.environ and os.environ["CI"] == "true"

    # In CI parameters.mode will be set
    print(f"parameters.mode = {parameters.mode}")
    mode = parameters.mode or RELEASE_MODE
    print(f"MODE = {mode}")

    build_selection = parameters.build_selection or BUILD_SELECTION_ALL

    # Base always builds
    base_component = tap.Component(name="sspl_base", base_version=get_package_version(base.PACKAGE_PATH))
    with stage.parallel('base'):
        if mode == RELEASE_MODE:
            base_build = stage.artisan_build(name=f"base_{RELEASE_MODE}",
                                             component=base_component,
                                             image=BUILD_TEMPLATE,
                                             mode=RELEASE_MODE,
                                             release_package=base.PACKAGE_PATH)

            stage.artisan_build(name=f"bazel",
                                component=base_component,
                                image=BUILD_TEMPLATE_BAZEL,
                                mode="bazel_release",
                                release_package=PACKAGE_PATH)

            if running_in_ci:
                base_analysis_build = stage.artisan_build(name=f"base_{ANALYSIS_MODE}",
                                                          component=base_component,
                                                          image=BUILD_TEMPLATE,
                                                          mode=ANALYSIS_MODE,
                                                          release_package=base.PACKAGE_PATH)

                base_999_build = stage.artisan_build(name=f"base_{NINE_NINE_NINE_MODE}",
                                                     component=base_component,
                                                     image=BUILD_TEMPLATE,
                                                     mode=NINE_NINE_NINE_MODE,
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
    with stage.parallel('plugins'):
        if mode == RELEASE_MODE:
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

                    edr_999_build = stage.artisan_build(name=f"edr_{NINE_NINE_NINE_MODE}",
                                                        component=edr_component,
                                                        image=BUILD_TEMPLATE,
                                                        mode=NINE_NINE_NINE_MODE,
                                                        release_package=PACKAGE_PATH_EDR)
            # Event Journaler
            if build_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_EJ]:
                ej_build = stage.artisan_build(name=f"ej_{RELEASE_MODE}",
                                               component=ej_component,
                                               image=BUILD_TEMPLATE,
                                               mode=RELEASE_MODE,
                                               release_package=PACKAGE_PATH_EJ)
                if running_in_ci:
                    ej_analysis_build = stage.artisan_build(name=f"ej_{ANALYSIS_MODE}",
                                                            component=ej_component,
                                                            image=BUILD_TEMPLATE,
                                                            mode=ANALYSIS_MODE,
                                                            release_package=PACKAGE_PATH_EJ)

                    ej_999_build = stage.artisan_build(name=f"ej_{NINE_NINE_NINE_MODE}",
                                                       component=ej_component,
                                                       image=BUILD_TEMPLATE,
                                                       mode=NINE_NINE_NINE_MODE,
                                                       release_package=PACKAGE_PATH_EJ)

            # AV
            if build_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_AV]:
                av_build = stage.artisan_build(name=f"av_{RELEASE_MODE}",
                                               component=av_component,
                                               image=BUILD_TEMPLATE,
                                               mode=RELEASE_MODE,
                                               release_package=PACKAGE_PATH_AV)
                if running_in_ci:
                    av_analysis_build = stage.artisan_build(name=f"av_{ANALYSIS_MODE}",
                                                            component=av_component,
                                                            image=BUILD_TEMPLATE,
                                                            mode=ANALYSIS_MODE,
                                                            release_package=PACKAGE_PATH_AV)

                    av_999_build = stage.artisan_build(name=f"av_{NINE_NINE_NINE_MODE}",
                                                       component=av_component,
                                                       image=BUILD_TEMPLATE,
                                                       mode=NINE_NINE_NINE_MODE,
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

    # NB: run_tests can be None when tap is run locally.
    if parameters.run_tests != False:
        with stage.parallel('testing'):
            if mode == RELEASE_MODE:
                if build_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_BASE]:
                    run_base_tests(stage, context, base_build, mode, parameters)

                if build_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_EDR]:
                    run_edr_tests(stage, context, edr_build, mode, parameters)

                if build_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_EJ]:
                    run_ej_tests(stage, context, ej_build, mode, parameters)

                if build_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_AV]:
                    run_av_tests(stage, context, av_build, mode, parameters)

            elif mode == COVERAGE_MODE:
                if build_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_BASE]:
                    run_base_coverage_tests(stage, context, base_coverage_build, mode, parameters)

                if build_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_EDR]:
                    run_edr_coverage_tests(stage, context, edr_coverage_build, mode, parameters)

                if build_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_EJ]:
                    run_ej_coverage_tests(stage, context, ej_coverage_build, mode, parameters)

                if build_selection in [BUILD_SELECTION_ALL, BUILD_SELECTION_AV]:
                    run_av_coverage_tests(stage, context, av_coverage_build, mode, parameters)

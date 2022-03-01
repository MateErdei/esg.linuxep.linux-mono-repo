import tap.v1 as tap

PACKAGE_PATH = "./release-package.xml"
BUILD_TEMPLATE = 'JenkinsLinuxTemplate7'
@tap.pipeline(version=1, component='linux-mono-repo')
def linux_mono_repo(stage: tap.Root, context: tap.PipelineContext, parameters: tap.Parameters):
    component = tap.Component(name='linux-mono-repo', base_version="1.0.0")
    release_mode = 'release'
    with stage.parallel('build'):
            stage.artisan_build(name=release_mode, component=component, image=BUILD_TEMPLATE,
                                                 mode=release_mode, release_package=PACKAGE_PATH)

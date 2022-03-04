import tap.v1 as tap

BUILD_TEMPLATE_LINUX = 'JenkinsLinuxTemplate7'

RELEASE_PACKAGE = "release-package.xml"


@tap.pipeline(root_sequential=False)
def versig(stage: tap.Root):
    component = tap.Component(name='versig', base_version='2.0.0')

    stage.artisan_build(name='versig_sspl',
                        component=component,
                        image=BUILD_TEMPLATE_LINUX,
                        release_package=RELEASE_PACKAGE
                        )
    
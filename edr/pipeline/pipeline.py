import tap.v1 as tap

@tap.pipeline(version=1, component='sspl-plugin-template')
def template_plugin(stage: tap.Root, parameters: tap.Parameters):
    component = tap.Component(name='sspl-plugin-template', base_version='1.0.0')

    release_mode = 'release'
    nine_nine_nine_mode = '999'
    zero_six_zero_mode = '060'

    mode = parameters.mode or release_mode

    template_build = None
    with stage.parallel('build'):
        if mode == release_mode:
            template_build = stage.artisan_build(name=release_mode, component=component, image='JenkinsLinuxTemplate5',
                                            mode=release_mode, release_package='./build-files/release-package.xml')
            nine_nine_nine_build = stage.artisan_build(name=nine_nine_nine_mode, component=component, image='JenkinsLinuxTemplate5',
                                                       mode=nine_nine_nine_mode, release_package='./build-files/release-package.xml')
            zero_six_zero = stage.artisan_build(name=zero_six_zero_mode, component=component, image='JenkinsLinuxTemplate5',
                                                mode=zero_six_zero_mode, release_package='./build-files/release-package.xml')
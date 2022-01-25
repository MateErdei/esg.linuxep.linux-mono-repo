import tap.v1 as tap


@tap.pipeline(version=1, component='sspl-thininstaller')
def sspl_thininstaller(stage: tap.Root, parameters: tap.Parameters):
    component = tap.Component(name='sspl-thininstaller', base_version='1.1.3')
    mode = parameters.mode or 'release'

    #export TAP_PARAMETER_MODE=release|analysis|coverage*(requires bullseye)
    _thininstaller_build = None
    with stage.parallel('build'):
        _thininstaller_build = stage.artisan_build(name=mode, component=component, image='JenkinsLinuxTemplate6',
                                        mode=mode, release_package='./release-package.xml')


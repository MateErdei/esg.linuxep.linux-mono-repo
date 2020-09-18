import tap.v1 as tap


@tap.pipeline(version=1, component='sspl-warehouse')
def sspl_warehouse(stage: tap.Root):
    component = tap.Component(name='sspl-warehouse', base_version='1.0.0')

    with stage.parallel('build'):
        _build = stage.artisan_build(name='release', component=component, image='Warehouse',
                                         mode='release', release_package='./build/release-package.xml')

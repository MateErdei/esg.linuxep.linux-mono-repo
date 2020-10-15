import tap.v1 as tap


@tap.pipeline(version=1, component='sspl-warehouse')
def sspl_warehouse(stage: tap.Root):
    component = tap.Component(name='sspl-warehouse', base_version='1.0.0')

    with stage.parallel('build'):
        _build = stage.artisan_build(name='develop', component=component, image='Warehouse',
                                         mode='release', release_package='./build/release-package.xml')

        _build = stage.artisan_build(name='edr-999', component=component, image='Warehouse',
                                     mode='release', release_package='./build/release-package-edr-999.xml')

        _build = stage.artisan_build(name='mdr-999', component=component, image='Warehouse',
                                     mode='release', release_package='./build/release-package-mdr-999.xml')

        _build = stage.artisan_build(name='edr-mdr-999', component=component, image='Warehouse',
                                     mode='release', release_package='./build/release-package-edr-mdr-999.xml')

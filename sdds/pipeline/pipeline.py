import tap.v1 as tap


def build_dev_warehouse(stage: tap.Root, name="release-package"):
    component = tap.Component(name='dev-warehouse-'+name, base_version='1.0.0')
    stage.artisan_build(name=name,
                        component=component,
                        image='Warehouse',
                        release_package='./build/dev.xml',
                        mode=name)


@tap.pipeline(root_sequential=False)
def warehouse(stage: tap.Root, context: tap.PipelineContext, parameters: tap.Parameters):
    mdr999 = parameters.mdr_999 != 'false'
    edr999 = parameters.edr_999 != 'false'

    with stage.parallel('build'):
        build_dev_warehouse(stage=stage, name="release-package")
        if edr999:
            build_dev_warehouse(stage=stage, name="release-package-edr-999")
        if mdr999:
            build_dev_warehouse(stage=stage, name="release-package-mdr-999")
        if edr999 and mdr999:
            build_dev_warehouse(stage=stage, name="release-package-edr-mdr-999")

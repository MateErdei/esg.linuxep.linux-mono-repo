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

    with stage.parallel('build'):
        build_dev_warehouse(stage=stage, name="release-package")
        build_dev_warehouse(stage=stage, name="release-package-edr-999")
        # build_dev_warehouse(stage=stage, name="release-package-mdr-999")
        # build_dev_warehouse(stage=stage, name="release-package-edr-mdr-999")

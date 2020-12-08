import tap.v1 as tap


def build_dev_warehouse(stage: tap.Root):
    component = tap.Component(name='dev-warehouse', base_version='1.0.0')
    stage.artisan_build(name='sdds2', component=component, image='Warehouse', release_package='./build/dev.xml', mode='sdds2')


@tap.pipeline(root_sequential=False)
def warehouse(stage: tap.Root, context: tap.PipelineContext, parameters: tap.Parameters):

    with stage.parallel('build'):
        build_dev_warehouse(stage=stage)

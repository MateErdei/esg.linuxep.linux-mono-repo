import tap.v1 as tap
import xml.etree.ElementTree as ET


BAZEL_TEMPLATE = 'win2019_x64_bazel_20200730'


def create_component(package_file_path: str, component_name: str) -> tap.Component:
    """Create a tap component using the version from the associated package file."""
    root = ET.parse(package_file_path).getroot()
    component = tap.Component(name=component_name, base_version=root.attrib['version'])
    return component


def build_winep_suites(stage: tap.Root):
    winep_suites = "winep_suites"
    winep_suites_package = './build/winep_suites.xml'

    winep_suites_component = create_component(winep_suites_package, winep_suites)

    stage.artisan_build(name=winep_suites, component=winep_suites_component, image=BAZEL_TEMPLATE,
                        mode='release_unified', release_package=winep_suites_package)


def build_dev_warehouse(stage: tap.Root):
    component = tap.Component(name='dev-warehouse', base_version='1.0.0')
    stage.artisan_build(name='sdds2', component=component, image='Warehouse', release_package='./build/dev.xml', mode='sdds2')


@tap.pipeline(root_sequential=False)
def warehouse(stage: tap.Root, context: tap.PipelineContext, parameters: tap.Parameters):

    with stage.parallel('build'):
        build_dev_warehouse(stage=stage)

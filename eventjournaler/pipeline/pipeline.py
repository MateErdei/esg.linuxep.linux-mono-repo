import tap.v1 as tap
import xml.etree.ElementTree as ET

def get_package_version(package_path):
    """ Read version from package.xml """
    package_tree = ET.parse(package_path)
    package_node = package_tree.getroot()
    return package_node.attrib['version']

PACKAGE_PATH = './build-files/release-package.xml'
PACKAGE_VERSION = get_package_version(PACKAGE_PATH)

@tap.pipeline(version=1, component='sspl-event-journaler-plugin')
def event_journaler(stage: tap.Root, parameters: tap.Parameters):
    component = tap.Component(name='sspl-event-journaler-plugin', base_version=PACKAGE_VERSION)

    release_mode = 'release'
    nine_nine_nine_mode = '999'
    zero_six_zero_mode = '060'

    mode = parameters.mode or release_mode

    release_build = None
    with stage.parallel('build'):
        if mode == release_mode:
            release_build = stage.artisan_build(name=release_mode, component=component, image='JenkinsLinuxTemplate5',
                                            mode=release_mode, release_package=PACKAGE_PATH)
            nine_nine_nine_build = stage.artisan_build(name=nine_nine_nine_mode, component=component, image='JenkinsLinuxTemplate5',
                                                       mode=nine_nine_nine_mode, release_package=PACKAGE_PATH)
            zero_six_zero = stage.artisan_build(name=zero_six_zero_mode, component=component, image='JenkinsLinuxTemplate5',
                                                mode=zero_six_zero_mode, release_package=PACKAGE_PATH)
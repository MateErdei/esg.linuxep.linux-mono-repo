import tap.v1 as tap
import xml.etree.ElementTree as ET
def get_package_version():
    """ Read version from package.xml """
    package_path = './release-package.xml'
    package_tree = ET.parse(package_path)
    package_node = package_tree.getroot()
    return package_node.attrib['version']
@tap.pipeline(version=1, component='sspl-thininstaller')
def sspl_thininstaller(stage: tap.Root, parameters: tap.Parameters):
    component = tap.Component(name='sspl-thininstaller', base_version=get_package_version())
    mode = parameters.mode or 'release'

    #export TAP_PARAMETER_MODE=release|analysis|coverage*(requires bullseye)
    _thininstaller_build = None
    with stage.parallel('build'):
        _thininstaller_build = stage.artisan_build(name=mode, component=component, image='JenkinsLinuxTemplate7',
                                        mode=mode, release_package='./release-package.xml')


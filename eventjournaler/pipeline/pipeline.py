import tap.v1 as tap
import xml.etree.ElementTree as ET
from tap._pipeline.tasks import ArtisanInput

def get_package_version(package_path):
    """ Read version from package.xml """
    package_tree = ET.parse(package_path)
    package_node = package_tree.getroot()
    return package_node.attrib['version']

PACKAGE_PATH = './build-files/release-package.xml'
PACKAGE_VERSION = get_package_version(PACKAGE_PATH)

def install_requirements(machine: tap.Machine):
    """ install python lib requirements """
    pip_install(machine, '-r', machine.inputs.test_scripts / 'requirements.txt')


def pip_install(machine: tap.Machine, *install_args: str):
    """Installs python packages onto a TAP machine"""
    pip_index = os.environ.get('TAP_PIP_INDEX_URL')
    pip_index_args = ['--index-url', pip_index] if pip_index else []
    pip_index_args += ["--no-cache-dir",
                       "--progress-bar", "off",
                       "--disable-pip-version-check",
                       "--default-timeout", "120"]
    machine.run('pip3', '--log', '/opt/test/logs/pip.log',
                'install', *install_args, *pip_index_args,
                log_mode=tap.LoggingMode.ON_ERROR)


def robot_task(machine: tap.Machine):
    try:
        install_requirements(machine)
        machine.run('python3', machine.inputs.test_scripts / 'RobotFramework.py', timeout=3600)
    finally:
        machine.run('python3', machine.inputs.test_scripts / 'move_robot_results.py')
        machine.output_artifact('/opt/test/logs', 'logs')
        machine.output_artifact('/opt/test/results', 'results')

def get_inputs(context: tap.PipelineContext, template_plugin_build: ArtisanInput, mode: str):
    test_inputs = None
    if mode == 'release':
        test_inputs = dict(
            test_scripts = context.artifact.from_folder('./TA'),
            template_plugin_sdds = template_plugin_build / 'template/SDDS-COMPONENT',
        )
    return test_inputs

@tap.pipeline(version=1, component='sspl-plugin-template')
def template_plugin(stage: tap.Root, context: tap.PipelineContext, parameters: tap.Parameters):
    component = tap.Component(name='sspl-plugin-template', base_version=PACKAGE_VERSION)

    release_mode = 'release'
    nine_nine_nine_mode = '999'
    zero_six_zero_mode = '060'

    mode = parameters.mode or release_mode

    template_build = None
    with stage.parallel('build'):
        if mode == release_mode:
            template_build = stage.artisan_build(name=release_mode, component=component, image='JenkinsLinuxTemplate5',
                                            mode=release_mode, release_package=PACKAGE_PATH)
            nine_nine_nine_build = stage.artisan_build(name=nine_nine_nine_mode, component=component, image='JenkinsLinuxTemplate5',
                                                       mode=nine_nine_nine_mode, release_package=PACKAGE_PATH)
            zero_six_zero = stage.artisan_build(name=zero_six_zero_mode, component=component, image='JenkinsLinuxTemplate5',
                                                mode=zero_six_zero_mode, release_package=PACKAGE_PATH)


    with stage.parallel('test'):
        machines = (
            ("ubuntu1804",
             tap.Machine('ubuntu1804_x64_server_en_us', inputs=get_inputs(context, template_build, mode), platform=tap.Platform.Linux)),
            ("centos77", tap.Machine('centos77_x64_server_en_us', inputs=get_inputs(context, template_build, mode), platform=tap.Platform.Linux)),
            ("centos82", tap.Machine('centos82_x64_server_en_us', inputs=get_inputs(context, template_build, mode), platform=tap.Platform.Linux)),
            # add other distros here
        )

    with stage.parallel('integration'):
        for template_name, machine in machines:
            stage.task(task_name=template_name, func=robot_task, machine=machine)
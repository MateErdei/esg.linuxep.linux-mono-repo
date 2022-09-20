
import os

from typing import Dict

import tap.v1 as tap
from tap._pipeline.tasks import ArtisanInput
from tap._backend import Input


def build_dev_warehouse(stage: tap.Root, name="release-package", image='Warehouse'):
    component = tap.Component(name='dev-warehouse-'+name, base_version='1.0.0')
    return stage.artisan_build(name=name,
                               component=component,
                               image=image,
                               release_package='./build/dev.xml',
                               mode=name)

def build_sdds3_warehouse(stage: tap.Root, mode="dev", image='centos79_x64_bazel_20211025'):
    component = tap.Component(name='sdds3-warehouse-'+mode, base_version='1.0.0')
    return stage.artisan_build(name=mode,
                               component=component,
                               image=image,
                               release_package='./build/dev.xml',
                               mode=mode)


def get_inputs(context: tap.PipelineContext, build: ArtisanInput) -> Dict[str, Input]:
    print(str(build))

    test_inputs = dict(
        test_scripts=context.artifact.from_folder('./TA'),
        repo=build / 'sdds3-repo',
        launchdarkly=build / 'sdds3-launchdarkly',
        thin_installer=context.artifact.from_component('linuxep.thininstaller',
           "develop", None, org='', storage='esg-build-tested') / 'build/output'
    )
    return test_inputs


def python(machine: tap.Machine):
    return "python3"


def robot_task(machine: tap.Machine):
    robot_task_with_env(machine)


def robot_task_with_env(machine: tap.Machine, environment=None, machine_name=None):
    if machine_name is None:
        machine_name = machine.template
    try:
        robot_exclusion_tags = ["MANUAL", "FAIL"]

        machine.run('bash',
                    machine.inputs.test_scripts / "bin/install_packages.sh",
                    os.environ.get('TAP_PIP_INDEX_URL', ""))
        machine.run(python(machine),
                    machine.inputs.test_scripts / 'bin/RobotFramework.py',
                    *robot_exclusion_tags,
                    environment=environment,
                    timeout=3600)
    finally:
        machine.run(python(machine), machine.inputs.test_scripts / 'bin/move_robot_results.py')
        machine.output_artifact('/opt/test/logs', 'logs')
        machine.output_artifact('/opt/test/results', 'results')


def run_tap_tests(stage: tap.Root, context: tap.PipelineContext, parameters: tap.Parameters, build):
    test_inputs = get_inputs(context, build)
    ubuntu1804_machine = tap.Machine('ubuntu1804_x64_server_en_us', inputs=test_inputs, platform=tap.Platform.Linux)
    stage.task(task_name='ubuntu1804_x64', func=robot_task, machine=ubuntu1804_machine)
    return


@tap.pipeline(root_sequential=False)
def warehouse(stage: tap.Root, context: tap.PipelineContext, parameters: tap.Parameters):
    spec_only = parameters.spec_only != 'false'
    if spec_only:
        return

    mdr999 = parameters.mdr_999 != 'false'
    edr999 = parameters.edr_999 != 'false'
    base999 = parameters.base_999 != 'false'
    query_pack = parameters.query_pack != 'false'
    zero_six_zero = parameters.zero_six_zero != 'false'
    run_tests = parameters.run_tests != 'false'

    with stage.parallel('build'):
        branch = context.branch
        is_release_branch = branch.startswith('release-') or branch.startswith('hotfix-')
        if not is_release_branch:
            build = build_dev_warehouse(stage=stage, name="release-package")
            if base999:
                build_dev_warehouse(stage=stage, name="release-package-base-999")
            if query_pack:
                build_dev_warehouse(stage=stage, name="release-package-query-pack")
            if edr999:
                build_dev_warehouse(stage=stage, name="release-package-edr-999")
            if mdr999:
                build_dev_warehouse(stage=stage, name="release-package-mdr-999")
            if edr999 and mdr999:
                build_dev_warehouse(stage=stage, name="release-package-edr-mdr-999")
            if zero_six_zero:
                build_dev_warehouse(stage=stage, name="release-package-060")
            build_dev_warehouse(stage=stage, name="localwarehouse", image='JenkinsLinuxTemplate7')
            run_tests
        else:
            build = False

        is_release_branch = branch.startswith('release-') or branch.startswith('hotfix-')

        if is_release_branch:
            do_dev = False
            do_prod = True
        else:
            do_dev = parameters.dev != 'false'
            do_prod = parameters.prod == 'true'

        with stage.parallel('sdds3'):
            if do_prod:
                build_sdds3_warehouse(stage=stage, mode="prod")
                buildsdds3 = False;
            if do_dev:
                buildsdds3 = build_sdds3_warehouse(stage=stage, mode="dev")
                build_sdds3_warehouse(stage=stage, mode="999")

    if run_tests and buildsdds3:
        run_tap_tests(stage, context, parameters, buildsdds3)

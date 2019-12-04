import tap.v1 as tap


def robot_task(machine: tap.Machine):
    try:
        machine.run('python', machine.inputs.test_scripts / 'RobotFramework.py')
    finally:
        machine.run('python', machine.inputs.test_scripts / 'debug_tap.py')
        machine.output_artifact('/opt/test/logs', 'logs')
        machine.output_artifact('/opt/test/results', 'results')


@tap.pipeline(version=1, component='edr_plugin')
def edr_plugin(stage: tap.Root, context: tap.PipelineContext):
    inputs = dict(
        test_scripts=context.artifact.from_folder('./TA')
    )

    with stage.group('test'):
        stage.task('robot_framework', robot_task, machine=tap.Machine('ubuntu1804_x64_server_en_us', inputs=inputs, platform=tap.Platform.Linux))


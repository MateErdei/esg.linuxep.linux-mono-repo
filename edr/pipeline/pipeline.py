import tap.v1 as tap


def robot_task(machine: tap.Machine):
    try:
        machine.run('python', machine.inputs.test_scripts / 'RobotFramework.py')
    finally:
        machine.run('python', machine.inputs.test_scripts / 'move_robot_results.py')
        machine.output_artifact('/opt/test/logs', 'logs')
        machine.output_artifact('/opt/test/results', 'results')


def get_inputs(context: tap.PipelineContext):
    print(str(context.artifact.))
    print(str(context.artifact.build()))
    test_inputs = dict(
        test_scripts=context.artifact.from_folder('./TA'),
        edr=context.artifact.build() / 'release' / 'output'
    )
    return test_inputs


@tap.pipeline(version=1, component='sspl-edr-plugin')
def sspl_edr_plugin(stage: tap.Root, context: tap.PipelineContext):
    # inputs = dict(
    #     test_scripts=context.artifact.from_folder('./TA')
    # )
    machine=tap.Machine('ubuntu1804_x64_server_en_us', inputs=get_inputs(context), platform=tap.Platform.Linux)
    with stage.group('test'):
        stage.task(task_name='ubuntu1804_x64', func=robot_task, machine=machine)


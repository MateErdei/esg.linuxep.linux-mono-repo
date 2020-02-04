import tap.v1 as tap


def unittest_task(machine: tap.Machine):
    try:
        machine.run('python', machine.inputs.scripts / 'PythonUnittest.py')
    finally:
        machine.output_artifact('C:\Test\Logs', 'logs')
        machine.output_artifact('C:\Test\Results', 'results')


def robot_task(machine: tap.Machine):
    try:
        machine.run('python', machine.inputs.scripts / 'RobotFramework.py')
    finally:
        machine.output_artifact('C:\Test\Logs', 'logs')
        machine.output_artifact('C:\Test\Results', 'results')


@tap.pipeline(version=1, component='example')
def example(stage: tap.Root, context: tap.PipelineContext):
    inputs = dict(
        scripts=context.artifact.from_folder('./TA')
    )

    with stage.group('test'):
        stage.task('python_unittest', unittest_task, machine=tap.Machine('win7_pro_en_us', inputs=inputs))
        stage.task('robot_framework', robot_task, machine=tap.Machine('win7_pro_en_us', inputs=inputs))


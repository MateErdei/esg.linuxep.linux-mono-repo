import json
import robot
import sys
import os

from pubtap.robotframework.tap_result_listener import tap_result_listener

def main():
    tags = {'include': [], 'exclude': []}
    log_files = ['log.html', 'output.xml', 'report.html']

    if os.environ.get('TEST'):
        robot_args['test'] = os.environ.get('TEST')
    if os.environ.get('SUITE'):
        robot_args['suite'] = os.environ.get('SUITE')
    if os.environ.get('COVFILE'):
        tags['exclude'].append('EXCLUDE_ON_COVERAGE')

    robot_args = {
        'path':  r'/opt/test/inputs/test_scripts',
        'name': 'integration',
        'loglevel': 'DEBUG',
        'consolecolors': 'ansi',
        'include': tags['include'],
        'exclude': tags['exclude'],
        'log': log_files[0],
        'output': log_files[1],
        'report': log_files[2],
        'suite': '*',
        'test': '*'
    }

    try:
        # Create the TAP Robot result listener.
        listener = tap_result_listener(robot_args['path'], tags, robot_args['name'])
    except json.decoder.JSONDecodeError:
        # When running locally you can not initialise the TAP Results Listener
        listener = None

    if listener is not None:
        robot_args['listener'] = listener

    sys.exit(robot.run(robot_args['path'], **robot_args))

if __name__ == '__main__':
    main()

import json
import robot
import sys

from pubtap.robotframework.tap_result_listener import tap_result_listener


def main():
    # Exclude OSTIA on TAP, since we are testing builds immediately, before they have a chance to be put in warehouses
    # Exclude MANUAL on TAP
    # Exclude STRESS on TAP; as some of the tests here will not be appropriate

    tags = {'include': ['INTEGRATION', 'PRODUCT'], 'exclude': ['OSTIA', 'MANUAL', 'STRESS']}
    log_files = ['log.html', 'output.xml', 'report.html']

    robot_args = {
        'path':  r'/opt/test/inputs/test_scripts/',
        'name': 'SSPLAV',
        'loglevel': 'DEBUG',
        'consolecolors': 'ansi',
        'include': tags['include'],
        'exclude': tags['exclude'],
        'log': log_files[0],
        'output': log_files[1],
        'report': log_files[2],
        'suite': '*'
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

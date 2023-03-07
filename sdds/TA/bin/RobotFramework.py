import json
import robot
import sys


def main(argv):
    exclude = argv
    tags = {'include': ['PACKAGE'], 'exclude': exclude}

    robot_args = {
        'path':  r'/opt/test/inputs/test_scripts/',
        'name': 'SSPL_warehouse',
        'loglevel': 'DEBUG',
        'consolecolors': 'ansi',
        'include': tags['include'],
        'exclude': tags['exclude'],
        'log': '/opt/test/logs/log.html',
        'output': '/opt/test/logs/output.xml',
        'report': '/opt/test/logs/report.html',
        'suite': '*'
    }

    try:
        # Create the TAP Robot result listener.
        from pubtap.robotframework.tap_result_listener import tap_result_listener
        listener = tap_result_listener(robot_args['path'], tags, robot_args['name'])
        robot_args['listener'] = listener
    except ImportError:
        print("pubtap not available")
    except json.decoder.JSONDecodeError:
        # When running locally you can not initialise the TAP Results Listener
        pass

    return robot.run(robot_args['path'], **robot_args)


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))

# Copyright 2019-2023 Sophos Limited. All rights reserved.
import argparse
import json
import robot
import sys
import os

from pubtap.robotframework.tap_result_listener import tap_result_listener

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--include', nargs='+', help='keywords to include')
    parser.add_argument('--exclude', nargs='+', help='keywords to exclude')
    parser.add_argument("--test", "--TEST", help="single test to run", default=os.environ.get('TEST', None))
    parser.add_argument("--suite", "--SUITE", help="single test suite to run", default=os.environ.get('SUITE', None))
    parser.add_argument("--debug", "--DEBUG", action="store_true", default=os.environ.get('DEBUG', False))
    args = parser.parse_args()

    tags = {'include': [], 'exclude': []}
    if args.include is not None:
        tags['include'] = args.include

    if args.exclude is not None:
        tags['exclude'] = args.exclude

    log_files = ['log.html', 'output.xml', 'report.html']

    os.environ["INPUT_DIRECTORY"] = "/opt/test/inputs"
    os.environ['WEBSOCKET_SERVER'] = os.path.join(os.environ["INPUT_DIRECTORY"], "websocket_server/utils/websocket_server")
    os.environ['SSPL_EDR_PLUGIN_SDDS'] = os.path.join(os.environ["INPUT_DIRECTORY"], "edr_sdds")
    open(os.path.join(os.environ["INPUT_DIRECTORY"], "testUtilsMarker"), 'a').close()
    robot_args = {
        'path':  os.path.join(os.environ["INPUT_DIRECTORY"], "test_scripts"),
        'name': 'integration',
        'loglevel': 'DEBUG',
        'consolecolors': 'ansi',
        'include': tags['include'],
        'exclude': tags['exclude'],
        'log': log_files[0],
        'output': log_files[1],
        'report': log_files[2],
        'test': '*',
        'suite': '*',
    }

    if args.test:
        robot_args['test'] = args.test
    if args.suite:
        robot_args['suite'] = args.suite

    try:
        # Create the TAP Robot result listener.
        listener = tap_result_listener(robot_args['path'], tags, robot_args['name'])
    except json.decoder.JSONDecodeError:
        # When running locally you can not initialise the TAP Results Listener
        listener = None

    if listener is not None:
        robot_args['listener'] = listener

    sys.path.append(os.path.join(os.environ["INPUT_DIRECTORY"], 'common_test_libs'))
    sys.exit(robot.run(robot_args['path'], **robot_args))

if __name__ == '__main__':
    main()
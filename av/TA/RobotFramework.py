#!/bin/env python3
# Copyright 2020-2023 Sophos Limited. All rights reserved.

import argparse
import json
import robot
import os
import sys

print("Python:", sys.executable)
print("Python version:", sys.version)

from pubtap.robotframework.tap_result_listener import tap_result_listener


class ExtendAction(argparse.Action):
    def __call__(self, parser, namespace, values, option_string=None):
        items = getattr(namespace, self.dest) or []
        items.extend(values)
        setattr(namespace, self.dest, items)


def main():

    parser = argparse.ArgumentParser()
    parser.register('action', 'extend', ExtendAction)
    parser.add_argument('--include', nargs='+', help='keywords to include', action="extend")
    parser.add_argument('--exclude', nargs='+', help='keywords to exclude', action="extend")
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

    robot_args = {
        'path':  r'/opt/test/inputs/test_scripts/',
        'name': 'SSPLAV',
        'loglevel': 'DEBUG',
        'removekeywords': 'wuks',
        'consolecolors': 'ansi',
        'include': tags['include'],
        'exclude': tags['exclude'],
        'log': log_files[0],
        'output': log_files[1],
        'report': log_files[2],
        'suite': '*',
        'test': '*'
    }

    if os.path.isfile("/tmp/BullseyeCoverageEnv.txt"):
        # Disable failing the test run if coverage is enabled
        robot_args["statusrc"] = False

    if args.test:
        robot_args['test'] = args.test
        print("Restricting to single test:", args.test)
    if args.suite:
        robot_args['suite'] = args.suite
        print("Restricting to single suite:", args.suite)

    print("AV robot args:", robot_args)

    try:
        # Create the TAP Robot result listener.
        listener = tap_result_listener(robot_args['path'], tags, robot_args['name'])
    except json.decoder.JSONDecodeError:
        # When running locally you can not initialise the TAP Results Listener
        listener = None

    if listener is not None:
        robot_args['listener'] = listener

    return robot.run(robot_args['path'], **robot_args)


if __name__ == '__main__':
    sys.exit(main())

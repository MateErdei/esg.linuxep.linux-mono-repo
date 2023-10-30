# Copyright 2020-2023 Sophos Limited. All rights reserved.
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
    args = parser.parse_args()

    tags = {'include': [], 'exclude': []}
    if args.include is not None:
        tags['include'] = args.include

    if args.exclude is not None:
        tags['exclude'] = args.exclude

    if os.environ.get('DEBUG'):
        tags['exclude'].append('BREAKS_DEBUG')
    if os.environ.get('COVERAGE'):
        tags['exclude'].append('EXCLUDE_ON_COVERAGE')
    if os.environ.get("BAZEL"):
        tags["exclude"].append("EXCLUDE_BAZEL")
    if os.environ.get("PLATFORM"):
        platform = os.environ["PLATFORM"]
        tags["exclude"].append(f"EXCLUDE_{platform}")
    log_files = ['log.html', 'output.xml', 'report.html']

    open("/opt/test/inputs/testUtilsMarker", 'a').close()
    path = r'/opt/test/inputs/test_scripts'
    robot_args = {
        'name': 'integration',
        'loglevel': 'DEBUG',
        'consolecolors': 'ansi',
        'include': tags['include'],
        'exclude': tags['exclude'],
        'log': log_files[0],
        'output': log_files[1],
        'report': log_files[2],
        'suite': '*'
    }

    if os.environ.get('TEST'):
        robot_args['test'] = os.environ.get('TEST')
    if os.environ.get('SUITE'):
        robot_args['suite'] = os.environ.get('SUITE')

    os.environ['SYSTEM_PRODUCT_TEST_OUTPUT'] = '/opt/test/inputs/system_test/'
    os.environ['BASE_DIST'] = '/opt/test/inputs/base_sdds'
    os.environ['RA_DIST'] = '/opt/test/inputs/ra_sdds'
    os.environ['OUTPUT'] = '/opt/test/inputs/base/'
    os.environ['WEBSOCKET_SERVER'] = '/opt/test/inputs/websocket_server'

    os.environ['THIN_INSTALLER_OVERRIDE'] = '/opt/test/inputs/thininstaller'
    os.environ['SDDS3_Builder'] = '/opt/test/inputs/sdds3_tools/sdds3-builder'
    os.chmod(os.environ['SDDS3_Builder'], 0o755)

    try:
        # Create the TAP Robot result listener.
        listener = tap_result_listener(path, tags, robot_args['name'])
    except json.decoder.JSONDecodeError:
        # When running locally you can not initialise the TAP Results Listener
        listener = None

    if listener is not None:
        robot_args['listener'] = listener

    sys.path.append('/opt/test/inputs/common_test_libs')
    sys.path.append('/opt/test/inputs/SupportFiles')
    sys.exit(robot.run(path, **robot_args))


if __name__ == '__main__':
    main()

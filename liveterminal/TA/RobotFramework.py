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

    log_files = ['log.html', 'output.xml', 'report.html']

    if os.environ.get('TEST'):
        robot_args['test'] = os.environ.get('TEST')
    if os.environ.get('SUITE'):
        robot_args['suite'] = os.environ.get('SUITE')
    if os.environ.get('COVFILE'):
        tags['exclude'].append('EXCLUDE_ON_COVERAGE')

    os.environ["INPUT_DIRECTORY"] = "/opt/test/inputs"
    os.environ["TEST_SCRIPT_PATH"] = fr"{os.environ['INPUT_DIRECTORY']}/test_scripts"
    os.environ['BASE_DIST'] = os.path.join(os.environ["INPUT_DIRECTORY"], "base")
    os.environ['SSPL_LIVERESPONSE_PLUGIN_SDDS'] = os.path.join(os.environ["INPUT_DIRECTORY"], "liveresponse")
    os.environ['WEBSOCKET_SERVER'] = os.path.join(os.environ["INPUT_DIRECTORY"], "pytest_scripts/utils/websocket_server")
    os.environ['SYSTEMPRODUCT_TEST_INPUT'] = os.environ["INPUT_DIRECTORY"]
    os.environ["SOPHOS_INSTALL"] = "/opt/sophos-spl"

    open(os.path.join(os.environ["INPUT_DIRECTORY"], "testUtilsMarker"), 'a').close()
    robot_args = {
        'path':  os.environ["TEST_SCRIPT_PATH"],
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

    sys.path.append(os.path.join(os.environ["INPUT_DIRECTORY"], "common_test_libs"))
    sys.exit(robot.run(robot_args['path'], **robot_args))

if __name__ == '__main__':
    main()

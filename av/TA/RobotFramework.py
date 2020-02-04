import robot

if __name__ == '__main__':
    robot.run_cli(['--loglevel', 'DEBUG', '--listener', 'pubtap.robotframework.tap_result_listener.tap_result_listener',
                   '--consolecolors', 'ansi', r'C:\Test\Inputs\scripts\RobotScripts'])
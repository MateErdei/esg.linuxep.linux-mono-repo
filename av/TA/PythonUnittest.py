import sys
import unittest
from pubtap.pyunittest_reporting.taprunner import TapRunner
from pubtap.pyunittest_reporting.resultreporter import ResultReporter

if __name__ == '__main__':
    with ResultReporter() as reporter:
        loader = unittest.defaultTestLoader.discover(r"C:\Test\Inputs\scripts\Unittests", pattern="*Tests.py")
        test_runner = unittest.TextTestRunner(verbosity=2)
        runner = TapRunner(test_runner, reporter)

        result = runner.run(loader)
        failures = len(result.failures) + len(result.errors)
        if failures:
            print('Errors:', len(result.errors))
            print('Failures:', len(result.failures))
        sys.exit(failures)

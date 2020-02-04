import unittest


class ExampleTestCase(unittest.TestCase):
    def test_passing(self):
        pass

    def test_error(self):
        raise RuntimeError('Oh no')
        # pass

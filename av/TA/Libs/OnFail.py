from robot.libraries.BuiltIn import BuiltIn
from robot.api import logger

class OnFail(object):
    def __init__(self):
        self.__m_actions = []

    def run_on_failure(self, keyword, args=None):
        self.__m_actions.append((keyword, args))

    def run_failure_functions_if_failed(self):
        builtin = BuiltIn()
        for (keyword, args) in self.__m_actions:
            logger.info("Running %s" % keyword)
            args = args or []
            builtin.run_keyword_if_test_failed(keyword, *args)

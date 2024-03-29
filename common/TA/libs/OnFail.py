# Copyright 2023-2024 Sophos Limited. All rights reserved.
from robot.libraries.BuiltIn import BuiltIn
from robot.api import logger
import robot.errors


DEFAULT_CLEANUP_ACTIONS = []


class OnFail(object):
    def __init__(self):
        self.__m_fail_actions = None
        self.__m_cleanup_actions = None
        self.__m_late_cleanup_actions = None
        self.__m_errors = None
        self.reset()
        self.__m_builtin = BuiltIn()

    def reset(self):
        self.__m_fail_actions = []
        self.__m_cleanup_actions = DEFAULT_CLEANUP_ACTIONS[:]
        self.__m_late_cleanup_actions = []
        self.__m_errors = []

    @staticmethod
    def register_default_cleanup_action(keyword, *args):
        global DEFAULT_CLEANUP_ACTIONS
        DEFAULT_CLEANUP_ACTIONS.append((keyword, args))

    def run_on_failure(self, keyword, *args):
        self.__m_fail_actions.append((keyword, args))

    def register_on_fail(self, keyword, *args):
        self.__m_fail_actions.append((keyword, args))

    def register_on_fail_if_unique(self, keyword, *args):
        for (k, a) in self.__m_fail_actions:
            if k == keyword and a == args:
                return False
        self.__m_fail_actions.append((keyword, args))
        return True

    def deregister_on_fail(self, keyword, *args):
        self.__m_fail_actions.remove((keyword, args))

    def register_cleanup(self, keyword, *args):
        self.__m_cleanup_actions.append((keyword, args))

    def register_cleanup_if_unique(self, keyword, *args):
        for (k, a) in self.__m_cleanup_actions:
            if k == keyword and a == args:
                return False
        self.__m_cleanup_actions.append((keyword, args))
        return True

    def deregister_cleanup(self, keyword, *args):
        try:
            self.__m_cleanup_actions.remove((keyword, args))
        except ValueError:
            logger.debug("Cleanup action: %s not present" % (str((keyword, args))))

    def deregister_optional_cleanup(self, keyword, *args):
        try:
            self.__m_cleanup_actions.remove((keyword, args))
        except ValueError:
            pass

    def register_late_cleanup(self, keyword, *args):
        self.__m_late_cleanup_actions.append((keyword, args))

    def __import_if_required(self, keyword):
        if keyword.startswith("CoreDumps."):
            self.__m_builtin.import_library("${COMMON_TEST_LIBS}/CoreDumps.py")

    def __run_actions(self, actions, if_failed=True):

        for (keyword, args) in reversed(actions):
            try:
                args = args or []
                self.__import_if_required(keyword)
                if if_failed:
                    self.__m_builtin.run_keyword_if_test_failed(keyword, *args)
                else:
                    self.__m_builtin.run_keyword(keyword, *args)
            except robot.errors.ExecutionPassed as err:
                err.set_earlier_failures(self.__m_errors)
                raise err
            except robot.errors.ExecutionFailed as err:
                self.__m_errors.extend(err.get_errors())
        actions.clear()

    def run_failure_functions_if_failed(self):
        self.__m_errors = []
        self.__run_actions(self.__m_fail_actions, True)
        if self.__m_errors:
            raise robot.errors.ExecutionFailures(self.__m_errors)

    def run_cleanup_functions(self):
        self.__m_errors = []
        self.__run_actions(self.__m_cleanup_actions, False)
        if self.__m_errors:
            raise robot.errors.ExecutionFailures(self.__m_errors)

    def run_teardown_functions(self):
        self.__m_errors = []
        self.__run_actions(self.__m_fail_actions, True)
        self.__run_actions(self.__m_cleanup_actions, False)
        self.__run_actions(self.__m_late_cleanup_actions, False)
        if self.__m_errors:
            raise robot.errors.ExecutionFailures(self.__m_errors)
        self.reset()


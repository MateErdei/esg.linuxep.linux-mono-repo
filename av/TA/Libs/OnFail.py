from robot.libraries.BuiltIn import BuiltIn


class OnFail(object):
    def __init__(self):
        self.__m_fail_actions = []
        self.__m_cleanup_actions = []

    def run_on_failure(self, keyword, *args):
        self.__m_fail_actions.append((keyword, args))

    def register_on_fail(self, keyword, *args):
        self.__m_fail_actions.append((keyword, args))

    def deregister_on_fail(self, keyword, *args):
        self.__m_fail_actions.remove((keyword, args))

    def register_cleanup(self, keyword, *args):
        self.__m_cleanup_actions.append((keyword, args))

    def deregister_cleanup(self, keyword, *args):
        self.__m_cleanup_actions.remove((keyword, args))

    def __run_actions(self, builtin, actions, if_failed=True):
        for (keyword, args) in reversed(actions):
            args = args or []
            if if_failed:
                builtin.run_keyword_if_test_failed(keyword, *args)
            else:
                builtin.run_keyword(keyword, *args)
        actions.clear()

    def run_failure_functions_if_failed(self):
        builtin = BuiltIn()
        self.__run_actions(builtin, self.__m_fail_actions, True)

    def run_cleanup_functions(self):
        builtin = BuiltIn()
        self.__run_actions(builtin, self.__m_cleanup_actions, False)

    def run_teardown_functions(self):
        builtin = BuiltIn()
        self.__run_actions(builtin, self.__m_fail_actions, True)
        self.__run_actions(builtin, self.__m_cleanup_actions, False)


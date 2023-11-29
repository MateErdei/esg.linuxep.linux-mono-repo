class Connection(object):
    def __init__(self):
        self.__m_appId = ""
        self.__m_policyId = ""
        self.__m_json_body = ""

    def get_policy(self, appId, policyId):
        self.__m_appId = appId
        self.__m_policyId = policyId

    def get_app_id(self):
        return self.__m_appId

    def get_policy_id(self):
        return self.__m_policyId

    def get_json_body(self):
        return self.__m_json_body

    def get_policy_fragment(self, appId, fragmentId):
        return "{}|{}|".format(appId, fragmentId)

    def send_v2_response_with_id(self, json_body, app_id, command_id):
        self.__m_appId = app_id
        self.__m_policyId = command_id
        self.__m_json_body = json_body
        return

class FakeCommand(object):
    def __init__(self,xml):
        self._m__dict = {"body":xml}
        self._m__complete = False
        self._m__connection = Connection()

    def get_connection(self):
        return self._m__connection

    def get(self, s):
        return self._m__dict.get(s, "")

    def get_xml_text(self):
        return ""

    def complete(self):
        self._m__complete = True

    def completed(self):
        return self.completed
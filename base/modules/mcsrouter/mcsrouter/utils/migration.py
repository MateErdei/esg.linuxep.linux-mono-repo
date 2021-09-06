# Copyright 2021 Sophos Plc, Oxford, England.
"""
migration message handling Module
"""

import xml.dom.minidom
import logging
import json
from . import path_manager
from . import xml_helper
from . import timestamp

LOGGER = logging.getLogger(__name__)

class Migrate(object):

    def __init__(self):
        self.token = None
        self.migrate_url = None

    def get_token(self):
        return self.token

    def get_migrate_url(self):
        return self.migrate_url

    def get_command_path(self):
        return "/v2/migrate"

    def read_migrate_action(self):
        try:
            doc = xml.dom.minidom.parse(path_manager.migration_action_path())
            action_node = doc.getElementsByTagName("action")[0]
            self.token = xml_helper.get_text_node_text(action_node, "token")
            self.migrate_url = xml_helper.get_text_node_text(action_node, "server")
        except Exception as e:
            LOGGER.error("Failed migrate with: {}".format(e))

    def create_failed_response_body(self, error):
        return """<event type="sophos.mgt.mcs.migrate.succeeded" ts="{}">
  <token>{}</token>
  <errorMessage>{}</errorMessage>
</event>""".format(timestamp.timestamp(), self.get_token(), error)

    def create_success_response_body(self):
        return """<event type="sophos.mgt.mcs.migrate.succeeded" ts="{}">
  <token>{}</token>
</event>""".format(timestamp.timestamp(), self.get_token())



# TODO after lunch! check:
#MIGRATION RESPONSE FROM NEW ACCOUNT: {"device_id": "ThisIsADeviceID+1002", "endpoint_id": "ThisIsAnMCSID+1002", "password": "ThisIsAPassword", "tenant_id": "ThisIsATenantID+1002"}
    def extract_values(self, response):
        logging.info(response)
        #{"device_id": "ThisIsADeviceID+1002", "endpoint_id": "ThisIsAnMCSID+1002", "password": "ThisIsAPassword", "tenant_id": "ThisIsATenantID+1002"}
        response_json = json.loads(response)
        return response_json['endpoint_id'], response_json['device_id'], response_json['tenant_id'], response_json['password']


import datetime
import logging
import os

import mcsrouter.adapters.generic_adapter as generic_adapter
import mcsrouter.adapters.adapter_base
import mcsrouter.utils.atomic_write
import mcsrouter.utils.path_manager as path_manager
import mcsrouter.utils.timestamp
import mcsrouter.utils.utf8_write
import mcsrouter.utils.xml_helper as xml_helper

LOGGER = logging.getLogger(__name__)

class LiveQueryAdapter(generic_adapter.GenericAdapter):
    """
    LiveQueryAdapter class override _process_action to handle processing of LiveQuery request
    LiveQuery request require that their file written to the actions folder has its name as:
     '<AppId=LiveQuery>_<correlation-id=id>_<timestamp=creationTime>_request.json'
    """
    def __init__(self, app_id, install_dir):
        generic_adapter.GenericAdapter.__init__(self, app_id, install_dir)

    def _process_action(self, command):
        """
        Override the process_action from the generic adapter
        """
        app_id = self.get_app_id()
        LOGGER.debug("{} received an action".format(app_id))

        body = command.get("body")
        max_bytes_size = 1024 * 1024
        if len(body) > max_bytes_size:
            LOGGER.error("Rejecting the query because it exceeds maximum allowed size. Query size ({}) bytes. "
                         "Allowed ({}) bytes.".format(len(body), max_bytes_size))
            return []
        correlation_id = command.get("id")
        try:
            timestamp = command.get("creationTime")
        except KeyError as ex:
            LOGGER.error("Could not get creationTime, using the timestamp. Exception: {}".format(ex))
            timestamp = mcsrouter.utils.timestamp.timestamp()

        order_tag = datetime.datetime.now().strftime("%Y%m%d%H%M%S%f")
        action_name = "{}_{}_{}_{}_request.json".format(order_tag, app_id, correlation_id, timestamp)
        action_path_tmp = os.path.join(path_manager.actions_temp_dir(), action_name)
        LOGGER.info("Query saved to path {}".format(action_path_tmp))
        mcsrouter.utils.utf8_write.utf8_write(action_path_tmp, body)
        return []

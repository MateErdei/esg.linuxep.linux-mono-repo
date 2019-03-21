
import logging
logger = logging.getLogger(__name__)

generic_true = (True, "true", "1")
generic_false = (False, "false", "0")


class PolicyHandlerBase(object):
    def __init__(self, install_dir):
        pass

    def config_differs(self, path, flat_config, debug_as_warning=False):
        raise NotImplementedError()

        log_debug = logger.debug
        if debug_as_warning:
            log_debug = logger.warning

        layer = self.m_layer
        (console_leaf, console_settings) = self.m_config_manager.retrieve(
            layer + "/" + path)
        (flat_leaf, flat_settings) = flat_config.retrieve(path)

        # Some special cases to handle differences that don't matter between
        # the interfaces
        if len(console_settings) == 0 and len(flat_settings) == 0:
            # Empty lists are the same regardless
            return False

        if console_settings == [""] and flat_settings == []:
            return False
        if console_settings == [] and flat_settings == [""]:
            return False

        if flat_leaf != console_leaf:
            log_debug(
                "CONFIG differs - leaf vs. non-leaf - %s - %s vs %s",
                path,
                str(console_settings),
                str(flat_settings))
            return True

        if len(console_settings) == 1 and len(flat_settings) == 1:
            if console_settings[0] in generic_false and flat_settings[0] in generic_false:
                return False

            if console_settings[0] in generic_true and flat_settings[0] in generic_true:
                return False

        if console_settings != flat_settings:
            log_debug("CONFIG differs for %s - %s vs %s", path,
                      str(console_settings), str(flat_settings))
            flat_config.debug()
            return True

        return False

    def is_compliant(self):
        """
        @return True if the endpoint is compliant with the policy
        """
        raise NotImplementedError()

        try:
            self.m_config_manager.openConnection()
            try:
                return self._checkCompliantWithOpenConnection()
            finally:
                self.m_config_manager.closeConnection()
        except mcsrouter.adapters.ConfigManager.ConfigManagerConnectionException:
            # Not compliant since savd isn't available
            return False

    def send_simple_event(self, action):
        raise NotImplementedError()

        event_factory = self.__m_event_factory
        event = event_factory.newSimpleEvent(action)
        self.m_config_manager.sendEvent(event)

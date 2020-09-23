import inspect
import sys

import json
import logging
logger = logging.getLogger("test_av_basic")
logger.setLevel(logging.DEBUG)


def test_av_plugin_can_receive_actions(sspl_mock, av_plugin_instance):
    logger.debug("Starting %s", inspect.currentframe().f_code.co_name)
    av_plugin_instance.start_av()
    agent = sspl_mock.management
    action_content = "DummyAction"
    agent.send_plugin_action('av', 'sav', "123", action_content)
    av_plugin_instance.wait_log_contains("Received new Action")
    logger.debug("Completed %s", inspect.currentframe().f_code.co_name)


def test_av_can_send_status(sspl_mock, av_plugin_instance):
    logger.debug("Starting %s", inspect.currentframe().f_code.co_name)
    av_plugin_instance.start_av()
    agent = sspl_mock.management
    status = agent.get_plugin_status('av', 'sav')
    assert "RevID" in status
    logger.debug("Completed %s", inspect.currentframe().f_code.co_name)


def test_av_can_send_telemetry(sspl_mock, av_plugin_instance):
    logger.debug("Starting %s", inspect.currentframe().f_code.co_name)
    av_plugin_instance.start_av()
    agent = sspl_mock.management
    av_telemetry = agent.get_plugin_telemetry('av')
    av_dict = json.loads(av_telemetry)
    assert "ml-lib-hash" in av_dict
    assert "version" in av_dict
    logger.debug("Completed %s", inspect.currentframe().f_code.co_name)

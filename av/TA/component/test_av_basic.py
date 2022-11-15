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
    agent.send_plugin_action('av', 'SAV', "123", action_content)
    av_plugin_instance.wait_log_contains("Received new Action")
    logger.debug("Completed %s", inspect.currentframe().f_code.co_name)


def test_av_can_send_status(sspl_mock, av_plugin_instance):
    logger.debug("Starting %s", inspect.currentframe().f_code.co_name)
    av_plugin_instance.start_av()
    agent = sspl_mock.management
    status = agent.get_plugin_status('av', 'SAV')
    assert "RevID" in status
    logger.debug("Completed %s", inspect.currentframe().f_code.co_name)


def test_av_can_send_telemetry(sspl_mock, av_plugin_instance):
    logger.debug("Starting %s", inspect.currentframe().f_code.co_name)
    av_plugin_instance.start_av()
    agent = sspl_mock.management
    av_telemetry = agent.get_plugin_telemetry('av')
    av_dict = json.loads(av_telemetry)
    assert "lr-data-hash" in av_dict
    assert "ml-lib-hash" in av_dict
    assert "vdl-ide-count" in av_dict
    assert "vdl-version" in av_dict
    assert "version" in av_dict
    assert "sxl4-lookup" in av_dict
    assert av_dict["lr-data-hash"] is not "unknown"
    assert av_dict["ml-lib-hash"] is not "unknown"
    assert av_dict["ml-pe-model-version"] is not "unknown"
    assert av_dict["vdl-ide-count"] is not "unknown"
    assert av_dict["vdl-version"] is not "unknown"
    assert av_dict["sxl4-lookup"] is True
    assert av_dict["scan-now-count"] == 0
    assert av_dict["scheduled-scan-count"] == 0
    assert av_dict["on-demand-threat-count"] == 0
    assert av_dict["on-demand-threat-eicar-count"] == 0
    assert av_dict["on-access-threat-count"] == 0
    assert av_dict["on-access-threat-eicar-count"] == 0

    logger.debug("Completed %s", inspect.currentframe().f_code.co_name)

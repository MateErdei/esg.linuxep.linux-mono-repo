import inspect

import logging

logger = logging.getLogger("test_scan_now")
logger.setLevel(logging.DEBUG)


def test_scan_now(sspl_mock, av_plugin_instance):
    logger.debug("Starting %s", inspect.currentframe().f_code.co_name)
    av_plugin_instance.start_av()
    agent = sspl_mock.management
    policy_content = r'<?xml version="1.0"?><config xmlns="http://www.sophos.com/EE/EESavConfiguration">' \
                     r'<csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/></config>'
    action_content = r'<?xml version="1.0"?><a:action xmlns:a="com.sophos/msys/action" type="ScanNow" id="" ' \
                     r'subtype="ScanMyComputer" replyRequired="1"/>'
    agent.send_plugin_policy('av', "sav", policy_content)
    agent.send_plugin_action('av', 'sav', "123", action_content)
    av_plugin_instance.wait_log_contains("Received new Action")
    av_plugin_instance.wait_log_contains("Evaluating Scan Now")
    av_plugin_instance.wait_log_contains("Starting scan Scan Now")
    av_plugin_instance.wait_log_contains("Sending scan complete event to Central")
    # TODO needs to check for reply from plugin
    logger.debug("Completed %s", inspect.currentframe().f_code.co_name)


def test_scan_now_no_config(sspl_mock, av_plugin_instance):
    logger.debug("Starting %s", inspect.currentframe().f_code.co_name)
    av_plugin_instance.start_av()
    agent = sspl_mock.management
    action_content = r'<?xml version="1.0"?><a:action xmlns:a="com.sophos/msys/action" type="ScanNow" id="" ' \
                     r'subtype="ScanMyComputer" replyRequired="1"/>'
    agent.send_plugin_action('av', 'sav', "123", action_content)
    av_plugin_instance.wait_log_contains("Received new Action")
    av_plugin_instance.wait_log_contains("Evaluating Scan Now")
    av_plugin_instance.wait_log_contains("Refusing to run invalid scan: INVALID")
    logger.debug("Completed %s", inspect.currentframe().f_code.co_name)

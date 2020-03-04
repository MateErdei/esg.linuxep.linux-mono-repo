import logging
def detect_failure(func):
    def wrapper_function(sspl_mock, edr_plugin_instance, caplog):
        try:
            caplog.set_level(logging.INFO)
            v = func(sspl_mock, edr_plugin_instance)
            return v
        except:
            edr_plugin_instance.set_failed()
            raise
    return wrapper_function

@detect_failure
def test_edr_plugin_can_receive_actions(sspl_mock, edr_plugin_instance):
    edr_plugin_instance.start_edr()
    agent = sspl_mock.management
    action_content = "test action"
    agent.send_plugin_action('edr', 'LiveQuery', "123", action_content)
    edr_plugin_instance.wait_log_contains("Received new Action")

@detect_failure
def test_edr_can_send_status(sspl_mock, edr_plugin_instance):
    edr_plugin_instance.start_edr()
    agent = sspl_mock.management
    status = agent.get_plugin_status('edr', 'LiveQuery')
    assert "RevID" in status
    edr_telemetry = agent.get_plugin_telemetry('edr')
    assert "osquery-database-purges" in edr_telemetry



def test_edr_plugin_receives_policy(sspl_mock, edr_plugin_instance):
    edr_plugin_instance.start_edr()
    agent = sspl_mock.management
    policy_text = 'test policy'
    agent.send_plugin_policy('edr', 'LiveQuery', policy_text)
    edr_plugin_instance.wait_log_contains(policy_text)


def test_edr_plugin_can_receive_actions(sspl_mock, edr_plugin_instance):
    edr_plugin_instance.start_edr()
    agent = sspl_mock.management
    action_content = "test action"
    agent.send_plugin_action('edr', 'LiveQuery', action_content)
    edr_plugin_instance.wait_log_contains("Received new Action")


def test_edr_can_send_status(sspl_mock, edr_plugin_instance):
    edr_plugin_instance.start_edr()
    agent = sspl_mock.management
    status = agent.get_plugin_status('edr', 'LiveQuery')
    assert "RevID" in status
    edr_telemetry = agent.get_plugin_telemetry('edr')
    assert "Number of Scans" in edr_telemetry


import json
import traceback
from common.messages import *
from common.SetupLogger import *
from ManagementAgentController import ManagementAgentController
from PluginController import PluginController
from FakeManagementAgent import Agent
from FakePlugin import FakePlugin


def assert_equal(expected, actual):
    if expected != actual:
        raise AssertionError("Not equal, expected:{}, actual:{}".format(expected, actual))


def assert_not_equal(left, right):
    if left == right:
        raise AssertionError("Expected not equal, left:{}, right:{}".format(left, right))


def main(argv):


    logger = setup_logging("PluginAndAgentTests", "PluginAndAgentTests")
    agent = Agent(logger)
    agent_controller = ManagementAgentController(MANAGEMENT_AGENT_CONTROLLER_SOCKET_PATH)

    update_plugin = FakePlugin(logger, "UpdatePlugin", "true")
    update_plugin.add_app_id("ALC")
    update_plugin_controller = PluginController(b"ipc://{}/fake_plugin_controller_{}.ipc".format(IPC_DIR, update_plugin.name))
    agent.link_app_id_to_plugin("ALC", update_plugin.name)

    update_plugin_2 = FakePlugin(logger, "UpdatePluginTwo", "true")
    update_plugin_2.add_app_id("ALC")
    agent.link_app_id_to_plugin("ALC", update_plugin_2.name)

    sav_plugin = FakePlugin(logger, "SAVPlugin", "false")
    sav_plugin.add_app_id("SAV")
    agent.link_app_id_to_plugin("SAV", sav_plugin.name)

    sav_plugin_controller = PluginController(b"ipc://{}/fake_plugin_controller_{}.ipc".format(IPC_DIR, sav_plugin.name))

    agent.start()
    # Added a small wait here so that the multi threaded registration happens in a set order to make validating the return messages easier.
    update_plugin.start()
    update_plugin_2.start()
    sav_plugin.start()
    try:
        sav_policy = "Example SAV policy"
        alc_policy = "Example ALC policy"
        ack = "ACK"
        status_xml = "StatusXML"
        status_without_xml = "StatusWithoutXML"
        telemetry = "Telemetry"

        sav_event = "SAV Event"
        alc_action = "ACL Action"

        plugins = [update_plugin.name, update_plugin_2.name, sav_plugin.name]
        # plugin requests to agent
        assert_equal(ack, sav_plugin_controller.register_plugin()[0])

        list_of_plugins = json.loads(agent_controller.get_registered_plugins()[0])
        assert_equal(sorted(list_of_plugins), sorted(plugins))

        assert_equal(ack, agent_controller.set_policy("SAV", sav_policy)[0])
        assert_equal(sav_policy, sav_plugin_controller.request_policy("SAV")[0])
        assert_equal(ack, sav_plugin_controller.send_event("SAV", sav_event)[0])
        assert_equal(ack, sav_plugin_controller.send_status("SAV", status_xml, status_without_xml)[0])

        plugin = None
        for p in agent.registered_plugins:
            if p.name == sav_plugin.name:
                plugin = p

        assert_not_equal(None, plugin)
        assert_equal(sav_event, plugin.get_events("SAV")[0])
        assert_equal([status_xml, status_without_xml], plugin.get_statuses("SAV")[0])

        # Agent requests to plugin
        assert_equal(ack, agent_controller.apply_policy("ALC", alc_policy)[0])
        assert_equal(alc_policy, update_plugin.get_policy())
        assert_equal(ack, agent_controller.do_action("ALC", update_plugin.name, alc_action)[0])
        assert_equal(alc_action, update_plugin.get_action())
        assert_equal(ack, sav_plugin_controller.set_status("SAV", status_xml, status_without_xml)[0])
        assert_equal([status_xml, status_without_xml], agent_controller.request_status("SAV", sav_plugin.name))
        assert_equal(ack, sav_plugin_controller.set_telemetry(telemetry)[0])
        assert_equal(telemetry, agent_controller.request_telemetry(sav_plugin.name)[0])

        assert_equal(ack, agent_controller.queue_reply(str(Messages.REQUEST_POLICY.value), "ALC", ["policy1"])[0])
        assert_equal(ack,  agent_controller.queue_reply(str(Messages.REQUEST_POLICY.value), "ALC", ["policy2"])[0])
        assert_equal(ack,  agent_controller.queue_reply(str(Messages.REQUEST_POLICY.value), "ALC", [Messages.ERROR.value, "no policy for you"])[0])
        assert_equal("policy1", update_plugin_controller.request_policy("ALC")[0])
        assert_equal("policy2", update_plugin_controller.request_policy("ALC")[0])
        assert_equal(Messages.ERROR.value, update_plugin_controller.request_policy("ALC")[0])

        queue_status_xml = "QueueStatusXML"
        queue_status_without_xml = "QueueStatusWithoutXML"
        assert_equal(ack, sav_plugin_controller.queue_reply(str(Messages.REQUEST_STATUS.value), "SAV", [queue_status_xml, queue_status_without_xml])[0])
        assert_equal([queue_status_xml, queue_status_without_xml], agent_controller.request_status("SAV", sav_plugin.name))
        assert_equal([status_xml, status_without_xml], agent_controller.request_status("SAV", sav_plugin.name))
    except Exception as ex:
        update_plugin.stop()
        update_plugin_2.stop()
        sav_plugin.stop()
        agent.stop()
        print ex
        traceback.print_exc()
        exit(1)


    update_plugin.stop()
    update_plugin_2.stop()
    sav_plugin.stop()
    agent.stop()


if __name__ == "__main__":
    sys.exit(main(sys.argv))

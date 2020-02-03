from ManagementAgentController import ManagementAgentController
from common.messages import *


# Globals
CONTROLLER = ManagementAgentController(b"ipc://{}/fake_management_controller.ipc".format(IPC_DIR))


# Commands available on the command line.
class Commands(Enum):
    DO_ACTION = b'PluginDoAction'
    APPLY_POLICY = b'PluginApplyPolicy'
    APPLY_POLICY_FILE = b'PluginApplyPolicyFromFile'
    REQUEST_STATUS = b'PluginRequestStatus'
    REQUEST_TELEMETRY = b'PluginRequestTelemetry'
    GET_REGISTERED_PLUGINS = b'AgentGetRegisteredPlugins'
    SET_POLICY = b'AgentSetPolicy'
    SET_POLICY_FILE = b'AgentSetPolicyFromFile'
    DEREGISTER_PLUGINS = b'DeregisterPlugins'
    QUEUE_REPLY = b'QueueReply'
    CUSTOM_MESSAGE_TO_PLUGIN = b'SendCustomMessageToPlugin'
    LINK_APPID_PLUGIN = b'LinkAppIDPlugin'
    STOP = b'Stop'


def get_control_message_enum(command):
    for enum in Commands:
        if command == enum.value:
            return enum
    return None

def print_usage():
    print "Usage"
    print "{} <APPID> <PluginName>".format(Commands.LINK_APPID_PLUGIN.value)
    print "{} <APPID> <PluginName>".format(Commands.REQUEST_STATUS.value)
    print "{} <PluginName> <Action>".format(Commands.DO_ACTION.value)
    print "{} <APPID> <PolicyString>".format(Commands.APPLY_POLICY.value)
    print "{} <APPID> <PolicyFile>".format(Commands.APPLY_POLICY_FILE.value)
    print "{} <PluginName>".format(Commands.REQUEST_TELEMETRY.value)
    print "{}".format(Commands.GET_REGISTERED_PLUGINS.value)
    print "{} <APPID> <PolicyString>".format(Commands.SET_POLICY.value)
    print "{} <APPID> <PolicyFile>".format(Commands.SET_POLICY_FILE.value)
    print "{}".format(Commands.DEREGISTER_PLUGINS.value)
    print "{} <PluginAPICommand> <Response> <Payload> <APPID>".format(Commands.QUEUE_REPLY.value)
    print "{} <PluginName> <ContentsA> <ContentsB> <Contents...>".format(Commands.CUSTOM_MESSAGE_TO_PLUGIN.value)
    print "{}".format(Commands.STOP.value)


def handle_actions(action, contents):

    # Must always be an action
    if action is None:
        print "Action to handle is None"
        return 1

    if action == Commands.REQUEST_STATUS:
        if len(contents) < 2:
            print "Please an APPID and plugin name"
            return
        app_id = contents[0]
        plugin_name = contents[1]
        return CONTROLLER.request_status(app_id, plugin_name)

    elif action == Commands.DO_ACTION:
        if len(contents) < 3:
            print "Please provide APPID, plugin name and action XML."
            return
        app_id = contents[0]
        plugin_name = contents[1]
        action_string = contents[2]
        return CONTROLLER.do_action(app_id, plugin_name, action_string)

    elif action == Commands.APPLY_POLICY:
        if len(contents) < 2:
            print "Please provide APPID, plugin name and policy XML."
            return
        app_id = contents[0]
        policy = contents[1]
        return CONTROLLER.apply_policy(app_id, policy)

    elif action == Commands.APPLY_POLICY_FILE:
        if len(contents) < 2:
            print "Please provide APPID, plugin name and policy XML file path."
            return

        app_id = contents[0]
        filename = contents[1]
        if not os.path.isfile(filename):
            print "Policy file does not exist: {}".format(filename)
            return
        with open(filename, "r") as f:
                policy = f.read()
        return CONTROLLER.apply_policy(app_id, policy)

    elif action == Commands.REQUEST_TELEMETRY:
        if len(contents) < 1:
            print "Please provide plugin name."
            return
        return CONTROLLER.request_telemetry(contents[0])

    elif action == Commands.GET_REGISTERED_PLUGINS:
        return CONTROLLER.get_registered_plugins()

    elif action == Commands.SET_POLICY:
        if len(contents) < 2:
            print "Please provide APPID and policy XML."
            return
        app_id = contents[0]
        policy = contents[1]
        return CONTROLLER.set_policy(app_id, policy)

    elif action == Commands.SET_POLICY_FILE:
        if len(contents) < 2:
            print "Please provide APPID and policy XML file path."
            return
        app_id = contents[0]
        filename = contents[1]
        if not os.path.isfile(filename):
            print "Policy file does not exist: {}".format(filename)
            return
        with open(filename, "r") as f:
            policy = f.read()
        return CONTROLLER.set_policy(app_id, policy)

    elif action == Commands.DEREGISTER_PLUGINS:
        return CONTROLLER.deregister_plugins()

    elif action == Commands.QUEUE_REPLY:
        plugin_api_cmd = ""
        app_id = ""
        payload = []
        if len(contents) > 0:
            plugin_api_cmd = contents[0]
        if len(contents) > 1:
            app_id = contents[1]
        if len(contents) > 2:
            payload = contents[2:]

        return CONTROLLER.queue_reply(plugin_api_cmd, app_id, payload)

    elif action == Commands.CUSTOM_MESSAGE_TO_PLUGIN:
        plugin_api_cmd = ""
        plugin_name = ""
        app_id = ""
        payload = []

        if len(contents) > 0:
            app_id = contents[0]
        if len(contents) > 1:
            plugin_name = contents[1]
        if len(contents) > 2:
            plugin_api_cmd = contents[2]
        if len(contents) > 3:
            payload = contents[3:]

        return CONTROLLER.send_custom_message(app_id, plugin_name, plugin_api_cmd, payload)
    elif action == Commands.STOP:
        CONTROLLER.stop()
    elif action == Commands.LINK_APPID_PLUGIN:
        if len(contents) < 2:
            print "Please provide APPID and plugin name."
            return 1
        app_id = contents[0]
        plugin_name = contents[1]
        CONTROLLER.link_appid_plugin(app_id, plugin_name)
    else:
        print "Unrecognised action to handle: {}".format(action)
        return 1


def main(argv):
    if len(argv) == 1:
        print_usage()
        return 1

    action_enum = None

    if len(argv) > 1:
        action_raw = argv[1]
        action_enum = get_control_message_enum(action_raw)
        if action_enum is None:
            print "Unrecognised action: {}".format(action_raw)
            exit(1)

    contents = argv[2:]
    print contents

    return handle_actions(action_enum, contents)


if __name__ == "__main__":
    sys.exit(main(sys.argv))

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
    REQUEST_HEALTH = b'PluginRequestHealth'
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
    print("Usage")
    print(f"{Commands.LINK_APPID_PLUGIN.value} <APPID> <PluginName>")
    print(f"{Commands.REQUEST_STATUS.value} <APPID> <PluginName>")
    print(f"{Commands.DO_ACTION.value} <PluginName> <Action>")
    print(f"{Commands.APPLY_POLICY.value} <APPID> <PolicyString>")
    print(f"{Commands.APPLY_POLICY_FILE.value} <APPID> <PolicyFile>")
    print(f"{Commands.REQUEST_TELEMETRY.value} <PluginName>")
    print(f"{Commands.GET_REGISTERED_PLUGINS.value}")
    print(f"{Commands.SET_POLICY.value} <APPID> <PolicyString>")
    print(f"{Commands.SET_POLICY_FILE.value} <APPID> <PolicyFile>")
    print(f"{Commands.DEREGISTER_PLUGINS.value}")
    print(f"{Commands.QUEUE_REPLY.value} <PluginAPICommand> <Response> <Payload> <APPID>")
    print(f"{Commands.CUSTOM_MESSAGE_TO_PLUGIN.value} <PluginName> <ContentsA> <ContentsB> <Contents...>")
    print(f"{Commands.STOP.value}")


def handle_actions(action, contents):
    # Must always be an action
    if action is None:
        print("Action to handle is None")
        return 1

    if action == Commands.REQUEST_STATUS:
        if len(contents) < 2:
            print("Please an APPID and plugin name")
            return
        app_id = contents[0]
        plugin_name = contents[1]
        return CONTROLLER.request_status(app_id, plugin_name)

    elif action == Commands.DO_ACTION:
        if len(contents) < 3:
            print("Please provide APPID, plugin name and action XML.")
            return
        app_id = contents[0]
        plugin_name = contents[1]
        action_string = contents[2]
        return CONTROLLER.do_action(app_id, plugin_name, action_string)

    elif action == Commands.APPLY_POLICY:
        if len(contents) < 2:
            print("Please provide APPID, plugin name and policy XML.")
            return
        app_id = contents[0]
        policy = contents[1]
        return CONTROLLER.apply_policy(app_id, policy)

    elif action == Commands.APPLY_POLICY_FILE:
        if len(contents) < 2:
            print("Please provide APPID, plugin name and policy XML file path.")
            return

        app_id = contents[0]
        filename = contents[1]
        if not os.path.isfile(filename):
            print(f"Policy file does not exist: {filename}")
            return
        with open(filename, "r") as f:
            policy = f.read()
        return CONTROLLER.apply_policy(app_id, policy)

    elif action == Commands.REQUEST_TELEMETRY:
        if len(contents) < 1:
            print("Please provide plugin name.")
            return
        return CONTROLLER.request_telemetry(contents[0])

    elif action == Commands.GET_REGISTERED_PLUGINS:
        return CONTROLLER.get_registered_plugins()

    elif action == Commands.SET_POLICY:
        if len(contents) < 2:
            print("Please provide APPID and policy XML.")
            return
        app_id = contents[0]
        policy = contents[1]
        return CONTROLLER.set_policy(app_id, policy)

    elif action == Commands.SET_POLICY_FILE:
        if len(contents) < 2:
            print("Please provide APPID and policy XML file path.")
            return
        app_id = contents[0]
        filename = contents[1]
        if not os.path.isfile(filename):
            print(f"Policy file does not exist: {filename}")
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
            print("Please provide APPID and plugin name.")
            return 1
        app_id = contents[0]
        plugin_name = contents[1]
        CONTROLLER.link_appid_plugin(app_id, plugin_name)
    else:
        print(f"Unrecognised action to handle: {action}")
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
            print(f"Unrecognised action: {action_raw}")
            exit(1)

    contents = argv[2:]
    print(contents)

    return handle_actions(action_enum, contents)


if __name__ == "__main__":
    sys.exit(main(sys.argv))

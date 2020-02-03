from PluginController import PluginController
from common.messages import *

# Globals
CONTROLLER = None


# Commands available on the command line.
class Commands(Enum):
    DO_REGISTRATION = b'Register'
    DO_SEND_EVENT = b'SendEvent'
    DO_SEND_EVENT_FILE = b'SendEventFile'
    DO_SEND_STATUS = b'SendStatus'
    DO_SEND_STATUS_FILE = b'SendStatusFile'
    DO_SET_STATUS = b'SetStatus'
    DO_SET_STATUS_FILE = b'SetStatusFile'
    DO_SET_TELEMETRY = b'SetTelemetry'
    DO_SET_TELEMETRY_FILE = b'SetTelemetryFile'
    DO_REQUEST_POLICY = b'RequestPolicy'
    QUEUE_REPLY = b'QueueReply'
    CUSTOM_MESSAGE_TO_AGENT = b'SendCustomMessageToManagementAgent'
    STOP = b'Stop'


def get_control_message_enum(command):
    for enum in Commands:
        if command == enum.value:
            return enum
    return None

def print_usage():
    print "Usage"
    print "<PluginName> {}".format(Commands.DO_REGISTRATION.value)
    print "<PluginName> {}".format(Commands.DO_REQUEST_POLICY.value)
    print "<PluginName> {} <APPID> <EventXmlString>".format(Commands.DO_SEND_EVENT.value)
    print "<PluginName> {} <APPID> <EventXmlFile>".format(Commands.DO_SEND_EVENT_FILE.value)
    print "<PluginName> {} <APPID> <StatusXmlString> <StatusWithoutXmlString>".format(Commands.DO_SEND_STATUS.value)
    print "<PluginName> {} <APPID> <StatusXmlFile> <StatusWithoutXmlString>".format(Commands.DO_SEND_STATUS_FILE.value)
    print "<PluginName> {} <APPID> <StatusXmlString> <StatusWithoutXmlString>".format(Commands.DO_SET_STATUS.value)
    print "<PluginName> {} <APPID> <StatusXmlFile> <StatusWithoutXmlString>".format(Commands.DO_SET_STATUS_FILE.value)
    print "<PluginName> {} <TelemetryString>".format(Commands.DO_SET_TELEMETRY.value)
    print "<PluginName> {} <TelemetryFile>".format(Commands.DO_SET_TELEMETRY_FILE.value)
    print "<PluginName> {} <PluginAPICommand> <Response> <Payload> <APPID>".format(Commands.QUEUE_REPLY.value)
    print "<PluginName> {} <ContentsA> <ContentsB> <Contents...>".format(Commands.CUSTOM_MESSAGE_TO_AGENT.value)
    print "{}".format(Commands.STOP.value)


def handle_actions(action, contents):

    # Must always be an action
    if action is None:
        print "Action to handle is None"
        return 1

    if action == Commands.DO_REGISTRATION:
        return CONTROLLER.register_plugin()

    elif action == Commands.DO_REQUEST_POLICY:
        return CONTROLLER.request_policy()

    elif action == Commands.DO_SEND_EVENT:
        if len(contents) < 1:
            print "Please provide event APPID."
            return
        if len(contents) < 2:
            print "Please provide event XML."
            return
        app_id = contents[0]
        event = contents[1]
        return CONTROLLER.send_event(app_id, event)

    elif action == Commands.DO_SEND_EVENT_FILE:
        if len(contents) < 1:
            print "Please provide event APPID."
            return
        if len(contents) < 2:
            print "Please provide event XML file path."
            return

        app_id = contents[0]
        filename = contents[1]

        if not os.path.isfile(filename):
            print "Event file does not exist: {}".format(filename)
            return

        with open(filename, "r") as f:
            event = f.read()

        return CONTROLLER.send_event(app_id, event)

    elif action == Commands.DO_SEND_STATUS:
        if len(contents) < 3:
            print "Please provide APPID, status XML and status without xml."
            return
        app_id = contents[0]
        status_xml = contents[1]
        status_without_xml = contents[2]
        return CONTROLLER.send_status(app_id, status_xml, status_without_xml)

    elif action == Commands.DO_SEND_STATUS_FILE:
        if len(contents) < 3:
            print "Please provide APPID, status XML file path and status without xml."
            return
        app_id = contents[0]
        filename = contents[1]
        if not os.path.isfile(filename):
            print "Event file does not exist: {}".format(filename)
            return
        with open(filename, "r") as f:
            status_xml = f.read()
        status_without_xml = contents[2]
        return CONTROLLER.send_status(app_id, status_xml, status_without_xml)

    elif action == Commands.DO_SET_STATUS:
        if len(contents) < 3:
            print "Please provide APPID, status XML and status without xml."
            return
        app_id = contents[0]
        status_xml = contents[1]
        status_without_xml = contents[2]
        return CONTROLLER.set_status(app_id, status_xml, status_without_xml)

    elif action == Commands.DO_SET_STATUS_FILE:
        if len(contents) < 3:
            print "Please provide APPID, status XML file path and status without xml."
            return
        app_id = contents[0]
        filename = contents[1]
        if not os.path.isfile(filename):
            print "Event file does not exist: {}".format(filename)
            return
        with open(filename, "r") as f:
            status_xml = f.read()
        status_without_xml = contents[2]
        return CONTROLLER.set_status(app_id, status_xml, status_without_xml)

    elif action == Commands.DO_SET_TELEMETRY:
        if len(contents) < 2:
            print "Please provide telemetry."
            return
        status_xml = contents[0]
        status_without_xml = contents[1]
        return CONTROLLER.set_status(status_xml, status_without_xml)

    elif action == Commands.DO_SET_TELEMETRY_FILE:
        if len(contents) < 2:
            print "Please provide telemetry file."
            return
        filename = contents[0]
        if not os.path.isfile(filename):
            print "Event file does not exist: {}".format(filename)
            return
        with open(filename, "r") as f:
            status_xml = f.read()
        status_without_xml = contents[1]
        return CONTROLLER.set_status(status_xml, status_without_xml)

    elif action == Commands.QUEUE_REPLY:
        plugin_api_cmd = ""
        response = ""
        payload = ""
        app_id = ""
        if len(contents) > 0:
            plugin_api_cmd = contents[0]
        if len(contents) > 1:
            response = contents[1]
        if len(contents) > 2:
            payload = contents[2]
        if len(contents) > 3:
            app_id = contents[3]
        return CONTROLLER.queue_reply(plugin_api_cmd, response, payload, app_id)

    elif action == Commands.CUSTOM_MESSAGE_TO_AGENT:
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
    else:
        print "Unrecognised action to handle: {}".format(action)
        return 1


def main(argv):
    global CONTROLLER

    if len(argv) < 3:
        print_usage()
        return 1

    CONTROLLER = PluginController(b"ipc://{}/fake_plugin_controller_{}.ipc".format(IPC_DIR, argv[1]))

    action_raw = argv[2]
    action_enum = get_control_message_enum(action_raw)
    if action_enum is None:
        print "Unrecognised action: {}".format(action_raw)
        exit(1)

    contents = argv[3:]
    print contents

    return handle_actions(action_enum, contents)


if __name__ == "__main__":
    sys.exit(main(sys.argv))

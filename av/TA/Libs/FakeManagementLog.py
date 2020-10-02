
import os

from Libs.PluginCommunicationTools import FakeManagementAgent

def get_fake_management_log_filename():
    return "fake_management_agent.log"

def get_fake_management_log_path():
    return os.path.join(FakeManagementAgent.get_log_dir(), get_fake_management_log_filename())

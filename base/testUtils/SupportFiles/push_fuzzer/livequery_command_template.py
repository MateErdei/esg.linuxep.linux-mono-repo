import sys
import os

current_dir = os.path.dirname(os.path.abspath(__file__))
fuzz_libs_dir = os.path.abspath(os.path.join(current_dir, os.pardir, 'fuzz_tests'))
sys.path.insert(1, fuzz_libs_dir)
import replace_types
#this is a hack to satisfy katnip.legos.xml which is 'broken' for python3 (23.01.20)
sys.modules['types'] = sys.modules['replace_types']

from kitty.model import Template, Static, String
from katnip.legos.xml import XmlElement, XmlAttribute
'''
use file to prepare a verify templates content from 'mcsrouter_policy_templates.py'
ref:https://katnip.readthedocs.io/en/stable/katnip.legos.xml.html
'''


live_query = \
    XmlElement(name="livequery_command", element_name="ns:commands",
               attributes=[
                   XmlAttribute(name="xmlns", attribute="xmlns:ns", value="http://www.sophos.com/xml/mcs/commands", fuzz_attribute=False, fuzz_value=False),
                   XmlAttribute(name='schemaVersion', attribute='schemaVersion',
                                value="1.0", fuzz_attribute=False, fuzz_value=False)
               ],
               content=[XmlElement(name="command", element_name="command",
                                   content=[
                                       XmlElement(name="id", element_name="id",
                                                  content="firstcommand", delimiter="\n"),
                                       XmlElement(name="appId", element_name="appId",
                                                  content="LiveQuery", delimiter="\n"),
                                       XmlElement(name="creationTime", element_name="creationTime",
                                                  content="FakeTime", delimiter="\n"),
                                       XmlElement(name="ttl", element_name="ttl",
                                                  content="PT10000S", delimiter="\n"),
                                       XmlElement(name="body", element_name="body",
                                                  content='{"type": "sophos.mgt.action.RunLiveQuery", "name": "Test Query Special", "query": "select name from processes"}', delimiter="\n")
                                   ], delimiter="\n")
                        ], delimiter="\n")

declaration_static = Static("<?xml version='1.0'?>", name="declaration")
declaration_fuzz = String("<?xml version='1.0'?>", name="declaration")
newline_static = Static("\n", name="newline")

livequery_command = Template(fields=[declaration_static, newline_static, live_query], name="livequery_command_fuzz")

### wake-up command ###
wake_up = XmlElement(name="wake_up", element_name="action",
                     attributes=[ XmlAttribute(name="type", attribute="type", value="sophos.mgt.action.GetCommands",
                                               fuzz_attribute=True, fuzz_value=True)])
wake_up_command = Template(fields=[declaration_static, newline_static, wake_up], name="wake_up_command_fuzz")


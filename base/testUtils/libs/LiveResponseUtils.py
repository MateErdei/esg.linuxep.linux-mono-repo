import os

import BaseInfo as base_info

TMP_ACTIONS_DIR = os.path.join("/tmp", "actions")
BASE_ACTION_DIR = os.path.join(base_info.get_install(), "base", "mcs", "action")
os.makedirs(TMP_ACTIONS_DIR, exist_ok=True)

def get_live_response_file(live_response_filepath=None):

    if live_response_filepath is None:
        import glob
        filelist = glob.glob(os.path.join( BASE_ACTION_DIR, "LiveTerminal_*.xml"))
        if filelist is None:
            raise AssertionError("no livequery response file found")

        reresponse_filepath = filelist[0]
        return reresponse_filepath

def create_live_response_action(wss_url="wss://lr.url/", thumbprint="thumbprint"):
    return """<?xml version="1.0"?><ns:commands xmlns:ns="http://www.sophos.com/xml/mcs/commands" schemaVersion="1.0">
        <command>
            <id>LR</id>
            <seq>1</seq>
            <appId>LiveTerminal</appId>
            <creationTime>FakeTime</creationTime>
            <ttl>PT10000S</ttl>'
            <body>
                &lt;?xml version='1.0'?&gt;
                &lt;action type="sophos.mgt.action.InitiateLiveTerminal"&gt;
                &lt;url&gt;{0}&lt;/url&gt;
                &lt;thumbprint&gt;{1}&lt;/thumbprint&gt;
            </body>
            </command>
        </ns:commands>""".format(wss_url, thumbprint)
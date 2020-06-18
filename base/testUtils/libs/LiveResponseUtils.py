import os
import glob
import datetime
import shutil
import LiveQueryUtils

import BaseInfo as base_info

TMP_ACTIONS_DIR = os.path.join("/tmp", "actions")
BASE_ACTION_DIR = os.path.join(base_info.get_install(), "base", "mcs", "action")
os.makedirs(TMP_ACTIONS_DIR, exist_ok=True)

def get_live_response_file():
    filelist = glob.glob(os.path.join( BASE_ACTION_DIR, "LiveTerminal_*.xml"))
    if filelist is None:
        raise AssertionError("no livequery response file found")
    return filelist[0]


def create_live_response_action(wss_url="wss://lr.url/", thumbprint="thumbprint", correlationId="correlation-id"):
    now = datetime.datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%S.%fZ")
    return f"""<?xml version="1.0"?><ns:commands xmlns:ns="http://www.sophos.com/xml/mcs/commands" schemaVersion="1.0">
        <command>
            <id>{correlationId}</id>
            <seq>1</seq>
            <appId>LiveTerminal</appId>
            <creationTime>{now}</creationTime>
            <ttl>PT10000S</ttl>'
            <body>&lt;?xml version="1.0"?&gt;&lt;action type="sophos.mgt.action.InitiateLiveTerminal"&gt;&lt;url&gt;{wss_url}&lt;/url&gt; \
                        &lt;thumbprint&gt;{thumbprint}&lt;/thumbprint&gt; \
                                        &lt;/action&gt; \
                                         </body>
            </command>
        </ns:commands>"""

def create_live_response_action_fake_cloud(wss_url="wss://lr.url/", thumbprint="thumbprint"):
    return f"""<?xml version='1.0'?><action type="sophos.mgt.action.InitiateLiveTerminal"><url>{wss_url}</url><thumbprint>{thumbprint}</thumbprint></action>"""

def get_correlation_id():
    import uuid
    return str(uuid.uuid4())

def run_live_response(query, correlationId):

    query_file_name = "LiveTerminal_{}_action_{}.json".format(correlationId, LiveQueryUtils.get_valid_creation_time_and_ttl())
    query_file_path = os.path.join(TMP_ACTIONS_DIR, query_file_name)
    with open(query_file_path, 'w') as action_file:
        action_file.write(query)
        LiveQueryUtils.make_file_readable_by_mcs(query_file_path)

    # Move query file into mcs action dir to be picked up by management agent
    shutil.move(query_file_path, BASE_ACTION_DIR)

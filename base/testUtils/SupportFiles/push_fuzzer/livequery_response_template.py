import sys
import os

current_dir = os.path.dirname(os.path.abspath(__file__))
fuzz_libs_dir = os.path.abspath(os.path.join(current_dir, os.pardir, 'fuzz_tests'))
sys.path.insert(1, fuzz_libs_dir)

import replace_types
#this is a hack to satisfy katnip.legos.xml which is 'broken' for python3 (23.01.20)
sys.modules['types'] = sys.modules['replace_types']

from katnip.legos.json import str_to_json

osquery_response_template= \
    '''{
        "type": "sophos.mgt.response.RunLiveQuery",
        "queryMetaData": {"durationMillis":53,"errorCode":0,"errorMessage":"OK","rows":1,"sizeBytes":204},
        "columnMetaData": [{"name":"hostname","type":"TEXT"},{"name":"cpu_brand","type":"TEXT"},
        {"name":"cpu_type","type":"TEXT"},
        {"name":"physical_memory","type":"BIGINT"},
        {"name":"hardware_vendor","type":"TEXT"},
        {"name":"hardware_model","type":"TEXT"},
        {"name":"os_name","type":"TEXT"},
        {"name":"os_version","type":"TEXT"},
        {"name":"build","type":"TEXT"},
        {"name":"platform","type":"TEXT"},
        {"name":"patch","type":"INTEGER"},
        {"name":"uptime_seconds","type":"BIGINT"}],
        "columnData": [["centos8.eng.sophos","Intel(R) Xeon(R) Gold 6126 CPU @ 2.60GHz","x86_64",2134530688,"VMware, Inc.","VMware Virtual Platform","CentOS Linux","CentOS Linux release 8.1.1911 (Core)","","rhel",1911,409478]]
    }'''

response_fuzz = str_to_json(json_str=osquery_response_template, name="response_fuzz")

# test template creation
if __name__ == '__main__':
    response_template = response_fuzz
    print("response  template")
    print(response_template.render().tobytes().decode())

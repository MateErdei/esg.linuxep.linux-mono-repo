import logging
import urllib.parse

import requests


def get_base_ra_request_url(region="us-west-2", env="qa"):
    return f"https://endpoint-actions.cloudstation.{region}.{env}.hydra.sophos.com/endpoint/v1/endpoints/actions"


def get_ra_request_auth():
    with open("/root/performance/ra_auth") as f:
        content = f.readlines()
        username = content[0].split("=")[1].strip()
        password = content[1].split("=")[1].strip()

    return requests.auth.HTTPBasicAuth(username, password)


def get_ra_request_headers(tenant_id="c5af50b4-cb0f-4749-a332-5c0441bf46bc"):
    return {
        "X-Tenant-ID": tenant_id,
        "Content-Type": "application/json"
    }


def get_response_action_status(region, env, tenant_id, action_id):
    url = urllib.parse.urljoin(get_base_ra_request_url(region=region, env=env), action_id)
    auth = get_ra_request_auth()
    request_headers = get_ra_request_headers(tenant_id)

    res = requests.get(url, auth=auth, headers=request_headers)

    if res.ok:
        return res.json()
    logging.error(f"Failed to get response action status: {res.text}")


def send_execute_command(region, env, tenant_id, endpoint_id, cmd):
    url = get_base_ra_request_url(region=region, env=env)
    auth = get_ra_request_auth()
    request_headers = get_ra_request_headers(tenant_id)
    request_body = {
        "type": "executeCommand",
        "expires": "PT50M",
        "executionTimeout": "PT300S",
        "endpointIds": [endpoint_id],
        "params": {"command": cmd}
    }

    res = requests.post(url, auth=auth, headers=request_headers, json=request_body)

    if res.ok:
        return res.json()
    logging.error(f"Failed to send execute command response action: {res.text}")

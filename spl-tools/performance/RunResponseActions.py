import logging
import time

import requests


def get_base_ra_request_url(region="us-west-2", env="qa"):
    return f"https://endpoint-actions.cloudstation.{region}.{env}.hydra.sophos.com/endpoint/v1/endpoints/actions"


def get_proxy_details():
    return {"https": "http://user:password@10.55.36.235:3129"}


def get_ra_request_auth():
    with open("/root/performance/ra_auth") as f:
        content = f.readlines()
        username = content[0].split("=")[1].strip()
        password = content[1].split("=")[1].strip()

    return requests.auth.HTTPBasicAuth(username, password)


def get_ra_request_headers(tenant_id):
    return {
        "X-Tenant-ID": tenant_id,
        "Content-Type": "application/json"
    }


def get_response_action_status(region, env, tenant_id, action_id):
    time.sleep(0.5)
    url = f"{get_base_ra_request_url(region=region, env=env)}/{action_id}"
    request_headers = get_ra_request_headers(tenant_id)

    res = requests.get(url, auth=get_ra_request_auth(), proxies=get_proxy_details(), headers=request_headers)

    if res.ok:
        return res.json()
    logging.error(f"Failed to get response action status: {res.text}")


def send_execute_command(region, env, tenant_id, endpoint_id, cmd):
    url = get_base_ra_request_url(region=region, env=env)
    request_headers = get_ra_request_headers(tenant_id)
    request_body = {
        "type": "executeCommand",
        "expires": "PT50M",
        "executionTimeout": "PT300S",
        "endpointIds": [endpoint_id],
        "params": {"command": cmd}
    }

    res = requests.post(url,
                        auth=get_ra_request_auth(),
                        proxies=get_proxy_details(),
                        headers=request_headers,
                        json=request_body)

    if res.ok:
        return res.json()
    logging.error(f"Failed to send execute command response action: {res.text}")


def upload_response_actions_file(region, env, tenant_id, endpoint_id, file_path):
    url = get_base_ra_request_url(region=region, env=env)
    request_headers = get_ra_request_headers(tenant_id)
    request_body = {
        "type": "uploadFileFromEndpoint",
        "expires": "PT30M",
        "executionTimeout": "PT30M",
        "endpointIds": [endpoint_id],
        "params": {
            "targetPath": file_path
        }
    }

    res = requests.post(url, auth=get_ra_request_auth(), proxies=get_proxy_details(), headers=request_headers,
                        json=request_body)

    if res.ok:
        return res.json()
    logging.error(f"Failed to send upload response action file: {res.text}")

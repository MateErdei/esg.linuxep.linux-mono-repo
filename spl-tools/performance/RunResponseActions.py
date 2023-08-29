import logging

import requests


def get_base_ra_request_url(region="us-west-2", env="qa"):
    return f"https://endpoint-actions.cloudstation.{region}.{env}.hydra.sophos.com/endpoint/v1/endpoints/actions"


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
    url = f"{get_base_ra_request_url(region=region, env=env)}/{action_id}"
    request_headers = get_ra_request_headers(tenant_id)

    while True:
        res = requests.get(url, auth=get_ra_request_auth(), headers=request_headers)
        res_status = res.json()["endpoints"][0]["status"]
        if res_status != "pending" and res_status != "waitingForInput":
            break

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

    res = requests.post(url, auth=get_ra_request_auth(), headers=request_headers, json=request_body)

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

    res = requests.post(url, auth=get_ra_request_auth(), headers=request_headers, json=request_body)

    if res.ok:
        return res.json()
    logging.error(f"Failed to send upload response action file: {res.text}")


def download_response_actions_file(region, env, tenant_id, endpoint_id, file_path, size, sha256):
    url = get_base_ra_request_url(region=region, env=env)
    request_headers = get_ra_request_headers(tenant_id)
    request_body = {
        "type": "downloadToEndpoint",
        "expires": "PT40M",
        "executionTimeout": "PT60S",
        "endpointIds": [endpoint_id],
        "params": {
            "targetPath": file_path,
            "sha256": sha256,
            "size": size
        }
    }

    res = requests.post(url, auth=get_ra_request_auth(), headers=request_headers, json=request_body)

    if res.ok:
        return res.json()
    logging.error(f"Failed to send download response action file: {res.text}")


def upload_file_to_client_bucket(url, file_to_download):
    num_retries = 5
    timeout = 60
    logging.debug(f"Trying to upload file to client bucket in {num_retries} amount of tries with {timeout} second timeouts each")
    for i in range(num_retries):
        try:
            res = requests.put(url, data=open(file_to_download, "rb"), timeout=timeout)

            if not res.ok:
                logging.error(f"Failed to upload file to client bucket: {res.text}")
                continue

            logging.info("Successfully uploaded file to client bucket")
            break
        except requests.exceptions.Timeout as e:
            logging.error("Timeout reached when trying to upload file to client bucket")
            logging.error(f"Error message: {e}")


def create_file_for_ra(filepath):
    with open(filepath, "wb") as f:
        f.truncate(1024 * 1024 * 10)  # 10MB

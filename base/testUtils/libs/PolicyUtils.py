
import json
import os
import PathManager
import xml.dom.minidom
from robot.api import logger

ALC_POLICY_TEMPLATE_PATH = os.path.join(PathManager.get_support_file_path(), "CentralXml/ALC_SDDS3Template.xml")

def get_version_from_policy(policy_file):
    policy = xml.dom.minidom.parse(policy_file)

    cloud_subscription = policy.getElementsByTagName("cloud_subscription")[0]
    cloud_subscription_fixed_version = cloud_subscription.getAttribute("FixedVersion")

    logger.info("cloud subscription fixed version: {}".format(cloud_subscription_fixed_version))
    return cloud_subscription_fixed_version

def populate_esm_fixed_version(name: str, token = ''):

    with open(ALC_POLICY_TEMPLATE_PATH) as f:
        policy = f.read()
        fixed_version = f'<fixed_version> \n <token>{token}</token> \n <name>{name}</name> \n </fixed_version>'
        policy = policy.replace("{{fixed_version}}", fixed_version)
        return policy

def read_token_from_warehouse_linuxep_json(path: str) -> str:
    with open(path, 'r') as f:
        linuxepjson = json.loads(f.read())
        token = linuxepjson['token']
        return token

def read_name_from_warehouse_linuxep_json(path: str) -> str:
    with open(path, 'r') as f:
        linuxepjson = json.loads(f.read())
        name = linuxepjson['name']
        return name
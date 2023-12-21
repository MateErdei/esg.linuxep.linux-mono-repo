import json
import datetime
import calendar
import os
import socket
import xml.etree.ElementTree as ET

from typing import List

import PathManager
import xml.dom.minidom
from robot.api import logger

ALC_POLICY_TEMPLATE_PATH = os.path.join(PathManager.get_support_file_path(), "CentralXml/ALC_Template.xml")
DEFAULT_CLOUD_SUBSCRIPTION = '<subscription Id="Base" RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED"/>'
CLOUD_SUBSCRIPTION_WITH_PAUSED = '<subscription Id="Base" RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED" FixedVersion="2022.1.0.40"/>'


def _get_delayed_update_time() -> str:
    tomorrow = datetime.date.today() + datetime.timedelta(days=1)
    tmrw_str = calendar.day_name[tomorrow.weekday()]
    return f'<delay_updating Day="{tmrw_str}" Time="12:00:00"/>'


def get_version_from_policy(policy_file):
    policy = xml.dom.minidom.parse(policy_file)

    cloud_subscription = policy.getElementsByTagName("cloud_subscription")[0]
    cloud_subscription_fixed_version = cloud_subscription.getAttribute("FixedVersion")

    logger.info("cloud subscription fixed version: {}".format(cloud_subscription_fixed_version))
    return cloud_subscription_fixed_version


def populate_alc_policy_with_fixed_version(name: str, token: str, cloudsub: str, delayupdate: str):
    fixed_version = f'<fixed_version> \n <token>{token}</token> \n <name>{name}</name> \n </fixed_version>'
    with open(ALC_POLICY_TEMPLATE_PATH) as f:
        policy = f.read()
        policy = policy.replace("{{fixed_version}}", fixed_version)
        policy = policy.replace("{{Subscriptions}}", cloudsub)
        policy = policy.replace("{{delay_updating}}", delayupdate)
        return policy


def populate_alc_policy_without_fixed_version(cloudsub='', delayupdate=''):
    with open(ALC_POLICY_TEMPLATE_PATH) as f:
        policy = f.read()
        policy = policy.replace("{{fixed_version}}", '')
        policy = policy.replace("{{Subscriptions}}", cloudsub)
        policy = policy.replace("{{delay_updating}}", delayupdate)
        return policy


def populate_cloud_subscription():
    return populate_alc_policy_without_fixed_version(DEFAULT_CLOUD_SUBSCRIPTION)


def populate_cloud_subscription_with_paused():
    return populate_alc_policy_without_fixed_version(CLOUD_SUBSCRIPTION_WITH_PAUSED)


def populate_cloud_subscription_with_scheduled():
    delayed_update = _get_delayed_update_time()
    return populate_alc_policy_without_fixed_version(DEFAULT_CLOUD_SUBSCRIPTION, delayed_update)


def populate_fixed_version_with_normal_cloud_sub(name: str, token=''):
    return populate_alc_policy_with_fixed_version(name, token, DEFAULT_CLOUD_SUBSCRIPTION, '')


def populate_fixed_version_with_paused_updates(name: str, token=''):
    return populate_alc_policy_with_fixed_version(name, token, CLOUD_SUBSCRIPTION_WITH_PAUSED, '')


def populate_fixed_version_with_scheduled_updates(name: str, token: str):
    delayed_update = _get_delayed_update_time()
    return populate_alc_policy_with_fixed_version(name, token, DEFAULT_CLOUD_SUBSCRIPTION, delayed_update)


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


def get_ips(url: str) -> List[str]:
    try:
        addr_info = socket.getaddrinfo(url, None)
        ips = list(set([i[4][0] for i in addr_info]))
        return ips
    except socket.gaierror:
        logger.error(f"Could not resolve {url}")
        return []


# Remove specified namespace from the xml document
def remove_namespace(doc, namespace):
    ns = "{" + namespace + "}"
    ns_length = len(ns)
    for elem in doc.iter():
        if elem.tag.startswith(ns):
            elem.tag = elem.tag[ns_length:]


def generate_isolation_policy_with_ci_exclusions(base_policy_path: str, generated_policy_path: str):
    # The following need to be allow-listed so we don't orphan any CI machines.
    urls_to_allow = [
        ("rabbitmq.sophos-ops.com", 5671),
        ("dev.rabbitmq.sophos-ops.com", 5671),
        ("stag.rabbitmq.sophos-ops.com", 5671),
        ("authproxy.sophos-ops.com", 443),
        ("dev.authproxy.sophos-ops.com", 443),
        ("stag.authproxy.sophos-ops.com", 443)
    ]

    ET.register_namespace('csc', "com.sophos\msys\csc")

    to_allow = []
    for url, port in urls_to_allow:
        ips = get_ips(url)
        for ip in ips:
            to_allow.append((ip, port))

    policy_root = ET.parse(base_policy_path).getroot()

    for child in policy_root.find(".//{http://www.sophos.com/xml/msys/NetworkThreatProtection.xsd}selfIsolation"):
        if "exclusions" in child.tag:
            for host, port in to_allow:
                exclusion = ET.SubElement(child, "exclusion", type="ip")
                # direction = ET.SubElement(exclusion, "direction")
                # direction.text = "out"
                remote_address = ET.SubElement(exclusion, "remoteAddress")
                remote_address.text = host
                # local_port = ET.SubElement(exclusion, "localPort")
                # local_port.text = str(port)
                remote_port = ET.SubElement(exclusion, "remotePort")
                remote_port.text = str(port)

    # TODO use indent once we're on python 3.9+
    # ET.indent(policy, space="\t", level=0)

    # Remove all namespaces from the xml as ET changes the csc namespace when we write the xml.
    remove_namespace(policy_root, "http://www.sophos.com/xml/msys/NetworkThreatProtection.xsd")
    remove_namespace(policy_root, "com.sophos\mansys\policy")
    remove_namespace(policy_root, "ns0")
    remove_namespace(policy_root, "ns2")

    policy = ET.ElementTree(policy_root)
    policy.write(generated_policy_path, encoding="utf-8")

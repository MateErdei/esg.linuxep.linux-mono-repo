import json
import datetime
import calendar
import os
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
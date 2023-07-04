
import os
import PathManager
import xml.dom.minidom
from robot.api import logger

ALC_POLICY_TEMPLATE_PATH = os.path.join(PathManager.get_support_file_path(), "CentralXml/ALC_SDDS3Template.xml")
#ALC_POLICY_TEMPLATE_PATH = os.path.dirname("/home/dev/gitrepos/esg.linuxep.everest-base/testUtils/SupportFiles/CentralXml/ALC_SDDS3Template.xml")

def get_version_from_policy(policy_file):
    policy = xml.dom.minidom.parse(policy_file)

    cloud_subscription = policy.getElementsByTagName("cloud_subscription")[0]
    cloud_subscription_fixed_version = cloud_subscription.getAttribute("FixedVersion")

    logger.info("cloud subscription fixed version: {}".format(cloud_subscription_fixed_version))
    return cloud_subscription_fixed_version

def populate_esm_fixed_version(name: str, token: str):

    with open(ALC_POLICY_TEMPLATE_PATH) as f:
        policy = f.read()
        fixed_version = f'<fixed_version> \n <token>' + token + f'</token> \n <name>' + name + f'</name> \n </fixed_version>'
        policy = policy.replace("{{fixed_version}}", fixed_version)
        return policy


# def populate_subscriptions():
#     with open(ALC_POLICY_TEMPLATE_PATH) as f:
#         policy = f.read()
#         whitelist_items = []
#         for sha256 in whitelist_sha256s:
#             whitelist_items.append(f'<item type="sha256">{sha256}</item>')
#         for path in whitelist_paths:
#             whitelist_items.append(f'<item type="path">{path}</item>')
#         policy = policy.replace("{{whitelistItems}}", "\n".join(whitelist_items))
#         return policy



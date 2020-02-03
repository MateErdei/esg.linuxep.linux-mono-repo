import xml.dom.minidom
from robot.api import logger

def get_version_from_policy(policy_file):
    policy = xml.dom.minidom.parse(policy_file)

    cloud_subscription = policy.getElementsByTagName("cloud_subscription")[0]
    cloud_subscription_fixed_version = cloud_subscription.getAttribute("FixedVersion")

    logger.info("cloud subscription fixed version: {}".format(cloud_subscription_fixed_version))
    return cloud_subscription_fixed_version


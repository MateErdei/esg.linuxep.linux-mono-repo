import os

from robot.api import logger
from robot.libraries.BuiltIn import BuiltIn

from BaseInfo import get_value_from_ini_file


def get_plugin_version(plugin_name):
    version_location = os.path.join(BuiltIn().get_variable_value("${SOPHOS_INSTALL}"),
                                    "plugins",
                                    plugin_name,
                                    "VERSION.ini")
    try:
        return get_value_from_ini_file(version_location, "PRODUCT_VERSION")
    except Exception as ex:
        logger.info("Could not find version - Returning empty string, error: {}".format(str(ex)))
        return ""

import json
import os
import PathManager

# Note that the default values set here are also reproduced in the Perform Install keyword in TestSulDownloader

def create_config(rigidname="ServerProtectionLinux-Base",
                  certificatePath=os.path.join(PathManager.get_support_file_path(), "sophos_certs/"), include_plugins=None, remove_entries=[],
                  **kwargs):
    """
    Create a JSON config to supply as input to SUL Downloader
    :param install_path: the target path into which the product should be installed (optional)
    :param rigidname: the rigidname for the warehouse (optional)
    :param certificatePath: the certificatePath for the warehouse (optional)
    :param remove_entries: list of keys that will be removed from the config file
    :param include_plugins: include the prefix Names section of the config file, default forces section to be removed (optional)
    :param kwargs any extra argument that the user wants to 'add', or 'replace' from the dictionary.
    :return: the JSON config as a string
    """
    config = {
        "sophosURLs": [
            "https://localhost:1233"
        ],
        "credential": {
            "username": "regruser",
            "password": "regrABC123pass"
        },
        "proxy": {
            "url": "noproxy:",
            "credential": {
                "username": "",
                "password": "",
                "proxyType": ""
            }
        },
        "primarySubscription": {
            "rigidName": "%s" %rigidname,
            "tag": "RECOMMENDED"
        },
        "products": [
            {"rigidName": "ServerProtectionLinux-Plugin-Example", "tag": "RECOMMENDED"}
        ],
        "features": ["CORE", "SENSORS"],
        "logLevel": "VERBOSE"
    }

    if include_plugins is None:
        del config['products']

    for entry in remove_entries:
        if entry in config:
            del config[entry]

    for key, value in kwargs.items():
        config[key] = value

    return config


def create_update_cache_config(install_path=None, 
                               rigidname="ServerProtectionLinux-Base",
                               certificatePath=os.path.join(PathManager.get_support_file_path(), "sophos_certs/"), 
                               include_plugins=None):
    """
    Create a JSON config to supply as input to SUL Downloader
    :param install_path: the target path into which the product should be installed (optional)
    :param rigidname: the rigidname for the warehouse (optional)
    :param certificatePath: the certificatePath for the warehouse (optional)
    :param include_plugins: include the prefix Names section of the config file, default forces section to be removed (optional)
    :return: the JSON config as a string
    """
    config = create_config(install_path, rigidname, certificatePath, include_plugins)

    config['updateCache'] = ["localhost:1235", "localhost:1236"]
    config['cacheUpdateSslPath'] = os.path.join(PathManager.get_support_file_path(), "https/ca")

    return config


def create_json_config_for_ostia_vshield():
    config = {
        "sophosURLs": [
            "https://ostia.eng.sophos/latest/Virt-vShield"
        ],
        "credential": {
            "username": "32d902304329d67379c6c124019ab0eb",
            "password": "32d902304329d67379c6c124019ab0eb"
        },
        "proxy": {
            "url": "noproxy:",
            "credential": {
                "username": "",
                "password": ""
            }
        },
        "systemSslPath": ":system:",
        "releaseTag": "RECOMMENDED",
        "baseVersion": "0",
        "primary": "THISPRODUCTISNOTTHERE",
        "logLevel": "VERBOSE"
    }

    return json.dumps(config, separators=(',', ': '), indent=4)


def data_to_json(config):
    """
    Convert an dictionary object to a JSON string
    :param config: the dictionary to convert
    :return: A JSON representation of the config provided
    """
    return json.dumps(config, separators=(',', ': '), indent=4)


def create_json_update_cache_config(install_path=None, **kwargs):
    """
    Create a JSON config to supply as input to SUL Downloader
    :param install_path: the target path into which the product should be installed (optional)
    :return: the JSON config as a string
    """

    config = create_update_cache_config(install_path, **kwargs)
    return data_to_json(config)

def create_json_config(**kwargs):
    """
    Create a JSON config to supply as input to SUL Downloader
    :param install_path: the target path into which the product should be installed (optional)
    :return: the JSON config as a string
    """

    config = create_config(**kwargs)
    return data_to_json(config)


def create_download_report(warehouse_status="SUCCESS", product_status="UPTODATE", product_count=1):
    """
    Create a JSON report to supply a custom previous report for SUL Downloader
    :param warehouse_status: Previous warehouse status reported (optional)
    :param product_status: Previous product status (optional)
    :param product_count: 1 or 0.  For the number of products that the report should have data for.
    :return: the JSON report as a string
    """

    report = { "startTime": "20180821 121220",
               "finishTime": "20180821 121220",
               "syncTime": "",
               "status": warehouse_status,
               "sulError": "",
               "errorDescription": "Uninstall failed",
               "urlSource": "https://localhost:1233",
               "products": [ { "rigidName": "ServerProtectionLinux-Base",
                               "productName": "ServerProtectionLinux-Base#0.5.0",
                               "downloadVersion": "0.5.0",
                               "errorDescription": "",
                               "productStatus": product_status },
                             ]
               }

    if product_count == 0:
        del report['products']

    return report


def create_json_download_report(**kwargs):
    """
    Create a JSON report to supply to SUL Downloader for customised previous report
    :param
    :return: the JSON report as a string
    """

    report = create_download_report(**kwargs)
    return data_to_json(report)


def check_string_contains_entries(content, *argv):
    content = str(content)
    for entry in argv:
        entry = str(entry)
        if entry not in content:
            raise AssertionError("Content: {}. It does not contain value:{} ".format(content, entry))


def check_string_does_not_contain(content, *argv):
    content = str(content)
    for entry in argv:
        entry = str(entry)
        if entry in content:
            raise AssertionError("Content: {}. Contains value:{} ".format(content, entry))

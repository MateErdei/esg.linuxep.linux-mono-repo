# Copyright 2019 Sophos Plc, Oxford, England.
"""
path_manager Module
"""

import os

INST = os.environ.get('INST_DIR', '/tmp/sophos-spl')


def install_dir():
    """
    install_dir
    """
    return INST

# base


def base_path():
    """
    base_path
    """
    return os.path.join(install_dir(), 'base')

# update


def update_path():
    """
    update_path
    """
    return os.path.join(base_path(), 'update')


def update_var_path():
    """
    update_var_path
    """
    return os.path.join(update_path(), 'var')

def update_report_path():
    """
    update_report_path
    """
    return os.path.join(update_var_path(), 'updatescheduler')

# base/mcs


def mcs_path():
    """
    mcs_path
    """
    return os.path.join(base_path(), 'mcs')


def root_ca_path():
    """
    root_ca_path
    """
    return os.path.join(mcs_path(), "certs/mcs_rootca.crt")


def ca_env_override_flag_path():
    """
    ca_env_override_flag_path
    """
    return os.path.join(mcs_path(), "certs/ca_env_override_flag")

# base/mcs/action


def action_dir():
    """
    action_dir
    """
    return os.path.join(mcs_path(), 'action')

# base/mcs/event


def event_dir():
    """
    event_dir
    """
    return os.path.join(mcs_path(), 'event')

# base/mcs/policy


def policy_dir():
    """
    policy_dir
    """
    return os.path.join(mcs_path(), 'policy')

def mcs_flags_file():
    """
    mcs_flags_file
    """
    return os.path.join(sophos_etc_dir(), "flags-mcs.json")

def warehouse_flags_file():
    """
    warehouse_flags_file
    """
    return os.path.join(sophos_etc_dir(), "flags-warehouse.json")

def combined_flags_file():
    """
    combined_flags_file
    """
    return os.path.join(policy_dir(), "flags.json")

def response_dir():
    """
    response_dir
    """
    return os.path.join(mcs_path(), 'response')


def datafeed_dir():
    """
    The directory which plugins write datafeed result files into for MCS to pick up and send
    """
    return os.path.join(mcs_path(), 'datafeed')


def mcs_policy_file():
    """
    mcs_policy_file
    """
    return os.path.join(policy_dir(), "MCS-25_policy.xml")


def sophos_config_file():
    """
    sophos_config_file
    """
    return os.path.join(policy_dir(), "sophos.config")

# base/mcs/status


def status_dir():
    """
    status_dir
    """
    return os.path.join(mcs_path(), 'status')

def status_cache_dir():
    """
    status_dir
    """
    return os.path.join(status_dir(), 'cache')



# base/pluginRegistry


def plugin_registry_path():
    """
    plugin_registry_path
    """
    return os.path.join(base_path(), "pluginRegistry")

# etc


def etc_dir():
    """
    etc_dir
    """
    return os.path.join(base_path(), "etc")


def root_config():
    """
    root_config
    """
    return os.path.join(etc_dir(), "mcs.config")


def log_conf_file():
    """
    log_conf_file
    """
    return os.path.join(etc_dir(), "logger.conf")


def install_options_file():
    """
    install_options_file
    """
    return os.path.join(etc_dir(), "install_options")

# etc/sophosspl


def sophos_etc_dir():
    """
    sophos_etc_dir
    """
    return os.path.join(etc_dir(), 'sophosspl')


def mcs_policy_config():
    """
    mcs_policy_config
    """
    return os.path.join(sophos_etc_dir(), "mcs_policy.config")

def mcs_current_proxy():
    """
    mcs_current_proxy
    """
    return os.path.join(sophos_etc_dir(), "current_proxy")

def sophosspl_config():
    """
    sophosspl_config
    """
    return os.path.join(sophos_etc_dir(), "mcs.config")

# logs/base


def base_log_dir():
    """
    base_log_dir
    """
    return os.path.join(install_dir(), "logs", "base")


def register_log():
    """
    register_log
    """
    return os.path.join(base_log_dir(), "register_central.log")

# logs/base/sophosspl


def sophosspl_log_dir():
    """
    sophosspl_log_dir
    """
    return os.path.join(base_log_dir(), "sophosspl")


def mcs_router_log():
    """
    mcs_router_log
    """
    return os.path.join(sophosspl_log_dir(), "mcsrouter.log")


def mcs_envelope_log():
    """
    mcs_envelope_log
    """
    return os.path.join(sophosspl_log_dir(), "mcs_envelope.log")

# var


def var_dir():
    """
    var_dir
    """
    return os.path.join(install_dir(), 'var')

# var/cache


def cache_dir():
    """
    cache_dir
    """
    return os.path.join(var_dir(), 'cache')


def fragmented_policies_dir():
    """
    fragmented_policies_dir
    """
    return os.path.join(cache_dir(), "mcs_fragmented_policies")

# var/lock-sophosspl


def lock_sophos():
    """
    lock_sophos
    """
    return os.path.join(var_dir(), "lock-sophosspl")


def mcs_router_pid_file():
    """
    mcs_router_pid_file
    """
    return os.path.join(lock_sophos(), "mcsrouter.pid")

# tmp


def temp_dir():
    """
    temp_dir
    """
    return os.path.join(install_dir(), 'tmp')

def policy_temp_dir():
    """
    policy_temp_dir for temporary writing of policies
    """
    return os.path.join(install_dir(), 'tmp', 'policy')

def actions_temp_dir():
    """
    actions_temp_dir for temporary writing of actions
    """
    return os.path.join(install_dir(), 'tmp', 'actions')

# base/lib:base/lib64


def get_installation_lib_path():
    """
    get_installation_lib_path
    """
    return os.path.join(install_dir(), "base", "lib") + \
        ":" + os.path.join(install_dir(), "base", "lib64")


def wdctl_bin_path():
    """
    wdctl_bin_path
    """
    return os.path.join(install_dir(), "bin", "wdctl")

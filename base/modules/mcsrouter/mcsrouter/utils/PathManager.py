import os

INST = os.environ.get('INST_DIR', '/tmp/sophos-spl')


def install_dir():
    return INST

# base


def base_path():
    return os.path.join(install_dir(), 'base')

# update


def update_path():
    return os.path.join(base_path(), 'update')


def update_var_path():
    return os.path.join(update_path(), 'var')

# base/mcs


def mcs_path():
    return os.path.join(base_path(), 'mcs')


def root_ca_path():
    return os.path.join(mcs_path(), "certs/mcs_rootca.crt")

# base/mcs/action


def action_dir():
    return os.path.join(mcs_path(), 'action')

# base/mcs/event


def event_dir():
    return os.path.join(mcs_path(), 'event')

# base/mcs/policy


def policy_dir():
    return os.path.join(mcs_path(), 'policy')


def mcs_policy_file():
    return os.path.join(policy_dir(), "MCS-25_policy.xml")


def sophos_config_file():
    return os.path.join(policy_dir(), "sophos.config")

# base/mcs/status


def status_dir():
    return os.path.join(mcs_path(), 'status')

# base/pluginRegistry


def plugin_registry_path():
    return os.path.join(base_path(), "pluginRegistry")

# base/var/run/sophosspl


def get_var_run_dir():
    return os.path.join(base_path(), "var", "run", "sophosspl")


def get_update_last_event_file():
    return os.path.join(get_var_run_dir(), "update.last_event")

# etc


def etc_dir():
    return os.path.join(base_path(), "etc")


def root_config():
    return os.path.join(etc_dir(), "mcs.config")


def log_conf_file():
    return os.path.join(etc_dir(), "mcsrouter.log.conf")


def mcs_router_conf():
    return os.path.join(etc_dir(), "mcsrouter.conf")

# etc/sophosspl


def sophos_etc_dir():
    return os.path.join(etc_dir(), 'sophosspl')


def mcs_policy_config():
    return os.path.join(sophos_etc_dir(), "mcs_policy.config")


def sophosspl_config():
    return os.path.join(sophos_etc_dir(), "mcs.config")

# logs/base


def base_log_dir():
    return os.path.join(install_dir(), "logs", "base")


def register_log():
    return os.path.join(base_log_dir(), "registerCentral.log")

# logs/base/sophosspl


def sophosspl_log_dir():
    return os.path.join(base_log_dir(), "sophosspl")


def mcs_router_log():
    return os.path.join(sophosspl_log_dir(), "mcsrouter.log")


def mcs_envelope_log():
    return os.path.join(sophosspl_log_dir(), "mcs_envelope.log")

# var


def var_dir():
    return os.path.join(install_dir(), 'var')

# var/cache


def cache_dir():
    return os.path.join(var_dir(), 'cache')


def fragmented_policies_dir():
    return os.path.join(cache_dir(), "mcs_fragmented_policies")

# var/lock-sophosspl


def lock_sophos():
    return os.path.join(var_dir(), "lock-sophosspl")


def mcs_router_pid_file():
    return os.path.join(lock_sophos(), "mcsrouter.pid")

# tmp


def temp_dir():
    return os.path.join(install_dir(), 'tmp')

# base/lib:base/lib64


def get_installation_lib_path():
    return os.path.join(install_dir(), "base", "lib") + \
        ":" + os.path.join(install_dir(), "base", "lib64")


def wdctl_bin_path():
    return os.path.join(install_dir(), "bin", "wdctl")

import os
import subprocess

import robot.libraries.BuiltIn


def get_variable(var_name, default_value=None):
    try:
        return robot.libraries.BuiltIn.BuiltIn().get_variable_value("${%s}" % var_name) or default_value
    except robot.libraries.BuiltIn.RobotNotRunningError:
        return os.environ.get(var_name, default_value)


SOPHOS_INSTALL = get_variable("SOPHOS_INSTALL", "/opt/sophos-spl")
SAFESTORE_DB_DIR = get_variable("SAFESTORE_DB_DIR", os.path.join(SOPHOS_INSTALL, "plugins", "av", "var", "safestore_db"))
SAFESTORE_DB_PATH = get_variable("SAFESTORE_DB_PATH",
                                 os.path.join(SOPHOS_INSTALL, "plugins", "av", "var", "safestore_db", "safestore.db"))
SAFESTORE_DB_PASSWORD_PATH = get_variable("SAFESTORE_DB_PASSWORD_PATH", os.path.join(SOPHOS_INSTALL, "plugins", "av", "var", "safestore_db",
                                                           "safestore.pw"))


def get_safestore_db_password_as_hexadecimal():
    with open(SAFESTORE_DB_PASSWORD_PATH, "r") as f:
        hex_string = f.read()
        return hex_string.encode('utf-8').hex()


def run_safestore_tool_with_args(*args):
    """
    Usage:
    -h               List this help.
    -l               List the content of the SafeStore database.
    -x=<path>        Export the quarantined file, if any, associated with a single
                        object given in objid. The file will be saved in an
                        obfuscated format under the specified path.
    -dbpath=<path>   Specify the path of the SafeStore database.
    -pass=<passkey>  Specify the password for the database. Must be in
                        hexadecimal form.
    -objid=<GUID>    Specify the unique object ID of an item to be restored.
                        Can be specified multiple times.
    -threatid=<GUID> Specify the threat ID belonging to an event. This option
                        will restore all files and registry keys associated with
                        that event.
    """
    env = os.environ.copy()
    env["LD_LIBRARY_PATH"] = "/opt/sophos-spl/base/lib64"
    password = get_safestore_db_password_as_hexadecimal()
    tool_path = os.path.join(get_variable("AV_TEST_TOOLS", "/opt/test/inputs/tap_test_output"), "ssr")
    cmd = [tool_path, f"-dbpath={SAFESTORE_DB_PATH}", f"-pass={password}", *args]

    result = subprocess.run(cmd, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

    if result.returncode != 0:
        raise AssertionError(f"Running {cmd} failed with: {result.returncode}, {result.stderr}")

    return result.stdout.decode()


def get_contents_of_safestore_database():
    return run_safestore_tool_with_args("-l")


def restore_threat_in_safestore_database_by_threatid(threat_id):
    return run_safestore_tool_with_args(f"-threatid={threat_id}")

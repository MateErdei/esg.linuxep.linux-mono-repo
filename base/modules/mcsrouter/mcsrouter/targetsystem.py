# -*- coding: utf-8 -*-

"""
TargetSystem Module
"""
# pylint: disable=too-many-lines

import glob
import json
import os
import re
import string
import sys
import socket
import subprocess
import urllib2
import time

DISTRIBUTION_NAME_MAP = {
    'debian': 'debian',
    'gentoo': 'gentoo',
    'mandrake': 'mandrake',
    'redhat': 'redhat',
    'slackware': 'slackware',
    'suse': 'suse',
    'opensuse': 'suse',
    'sunjavadesktop': 'sjd',
    'turbolinux': 'turbo',
    'unitedlinux': 'suse',
    'fedora': 'fedora',
    'ubuntu': 'ubuntu',
    'asianux': 'asianux',
    'oracle': 'oracle',
    'centos': 'centos',
    'amazonlinux': 'amazon',

    # Sophos Anti-Virus for VMWare platforms.
    # NB, there is also a vshield-1.0 which can be assigned by detect_distro
    # and which corresponds to the "special" 1.0 release of SAV for vShield which
    # didn't have a dedicated update location.
    'sophosanti-virusforvmwarevshield': 'vshield',
}

#-----------------------------------------------------------------------------
# class TargetSystem
#-----------------------------------------------------------------------------


class TargetSystem(object):
    """
    Represents the system into which we are trying to install/load TALPA.
    """
    # pylint: disable=too-many-instance-attributes, no-self-use
    # pylint: disable=too-many-public-methods, too-many-return-statements

    class TargetDetectionException(Exception):
        """
        TargetDetectionException
        """
        pass

    def __init__(self, install_dir="."):
        """
        __init__
        """
        self._save_uname()
        self.platform = self.uname[0].lower().replace("-", "")
        self.is_linux = (self.platform == "linux")
        self.is_solaris = (self.platform == "sunos")
        self.is_hpux = (self.platform == "hpux")
        self.is_free_bsd = (self.platform == "freebsd")
        self.is_aix = (self.platform == "aix")
        self.m_vendor = None
        self.m_os_version = None
        self.m_upgrade_response = []
        self.m_install_dir = install_dir

        # First check if lsb_release is available
        # Avoid checking for oracle since this generated a very long binary pack name
        # Use oracle-release instead
        self.__collect_lsb_release()

        self.ambiguous = None
        self.product = None
        self.distro = self.detect_distro()
        self.kernel = self.detect_kernel()
        self.smp = self.detect_smp()
        self.arch = self.detect_architecture()
        self.version = self.detect_version()
        self.aws_instance_info = self.detect_instance_info(
            install_dir=install_dir)
        self.syscall_table = self.get_symbol_address('sys_call_table')
        if self.arch == 'x86_64':
            self.syscall_32_table = self.get_symbol_address(
                'ia32_sys_call_table')
        else:
            self.syscall_32_table = ''

        self.is_ubuntu = (self.distro == "ubuntu")
        self.is_redhat = (self.distro == "redhat")

        # we only use upstart on Ubuntu
        self.configure_upstart = self.is_ubuntu and self.version_is_at_least(
            "10.04") and os.path.isdir("/etc/init")
        self.configure_systemd = os.path.isdir("/usr/lib/systemd/system")\
            or os.path.isdir("/lib/systemd/system")

        self.has_upstart = self.configure_upstart and os.path.isfile(
            "/sbin/initctl")
        self.has_systemd = self.configure_systemd and os.path.isfile(
            "/bin/systemctl")

        # Ubuntu 15.04 (and 14.10) can have *both* upstart and systemd installed concurrently.
        # We need to detect the active mechanism.
        if self.has_upstart and self.has_systemd:
            init_process = self.__detect_init_process()
            self.use_systemd = (init_process == "systemd")
            self.use_upstart = (
                init_process == "init" or init_process == "upstart")
        else:
            self.use_upstart = self.has_upstart
            self.use_systemd = self.has_systemd

        self.use_smf = self.is_solaris and self.version_is_at_least("5.10")

        # True if daemons started asynchronously, so longer waits required for
        # savd to activate
        self.async_start = self.use_upstart or self.use_systemd

        self.os_name = self.product
        os_name_release_re = re.compile(r" release ", re.I)
        # Make the name shorter if possible
        self.os_name = os_name_release_re.sub(" ", self.os_name)

        self.__detect_os_version()

    def _read_uname(self):
        """
        _read_uname
        :return:
        """
        try:
            return os.uname()
        except OSError:  # os.uname() can fail on HP-UX with > 8 character hostnames
            return socket.gethostname()

    def _save_uname(self):
        """
        _save_uname
        :return:
        """
        self.uname = self._read_uname()
        self.last_uname_save = time.time()

    def __back_tick(self, command):
        """
        __back_tick
        :return:
        """
        if isinstance(command, basestring):
            command = [command]
        # Clean environment
        env = os.environ.copy()

        # Need to reset the python variables in case command is a python script

        def reset_env(env, variable):
            """
            reset_env
            :return:
            """
            orig = "ORIGINAL_%s" % variable
            if orig in env and env[orig] != "":
                env[variable] = env[orig]
            else:
                env.pop(variable, None)  # Ignore the variable if it's missing

        def remove_env_variable(env, variable):
            """
            remove_env_variable
            :return:
            """
            env.pop(variable, None)

        reset_env(env, 'PYTHONPATH')
        reset_env(env, 'LD_LIBRARY_PATH')
        reset_env(env, 'PYTHONHOME')

        def attempt(command, env, shell=False):
            """
            attempt
            :return:
            """
            try:
                proc = subprocess.Popen(command, env=env,
                                        stdout=subprocess.PIPE,
                                        stderr=subprocess.PIPE,
                                        shell=shell)
                (stdout, stderr) = proc.communicate()  # pylint: disable=unused-variable
                ret_code = proc.wait()
                assert ret_code is not None
            except OSError:
                ret_code = -1
                stdout = ""
            return ret_code, stdout

        (ret_code, stdout) = attempt(command, env)
        if ret_code == 0:
            return ret_code, stdout

        # Maybe we tried to execute a Python program with incorrect environment
        # so lets try again without any.
        clear_env = env.copy()
        remove_env_variable(clear_env, 'PYTHONPATH')
        remove_env_variable(clear_env, 'LD_LIBRARY_PATH')
        remove_env_variable(clear_env, 'PYTHONHOME')

        (ret_code, stdout) = attempt(command, clear_env)
        if ret_code == 0:
            return (ret_code, stdout)

        # Try via the shell

        (ret_code, stdout) = attempt(command, env, shell=True)
        if ret_code == 0:
            return (ret_code, stdout)
        (ret_code, stdout) = attempt(command, clear_env, shell=True)
        if ret_code == 0:
            return (ret_code, stdout)

        # Try with restricted PATH
        env['PATH'] = "/usr/bin:/bin:/usr/sbin:/sbin"
        (ret_code, stdout) = attempt(command, env)
        if ret_code == 0:
            return (ret_code, stdout)
        clear_env['PATH'] = env['PATH']
        (ret_code, stdout) = attempt(command, clear_env)
        if ret_code == 0:
            return (ret_code, stdout)

        return (ret_code, stdout)

    def using_lsb_release(self):
        """
        using_lsb_release
        :return:
        """
        if self.m_lsb_release:
            return True
        return False

    def __collect_lsb_release(self):
        """
        Collect any lsb_release info
        """
        # pylint: disable=too-many-locals, too-many-branches, too-many-statements
        self.m_lsb_release = {}

        if os.path.isfile('/etc/oracle-release'):
            return
        path = os.environ.get("PATH", "")
        lsb_release_exes = []
        for split_path_element in path.split(":"):
            lsb_release = os.path.join(split_path_element, "lsb_release")
            if os.path.isfile(lsb_release):
                lsb_release_exes.append(lsb_release)

        if "/usr/bin/lsb_release" not in lsb_release_exes \
                and os.path.isfile("/usr/bin/lsb_release"):
            lsb_release_exes.append("/usr/bin/lsb_release")

        ret_code = -2
        for lsb_release_exe in lsb_release_exes:
            (ret_code, stdout) = self.__back_tick([lsb_release_exe, '-a'])
            if ret_code == 0:
                break

        if ret_code != 0:
            # failed to run lsb_release
            return

        lines = stdout.split("\n")
        # Each line consists of "Key:<whitespace>*<value>"
        matcher = re.compile(r"([^:]+):\s*(.+)$")
        for line in lines:
            line_matched = matcher.match(line)
            if line_matched:
                self.m_lsb_release[line_matched.group(
                    1)] = line_matched.group(2)

        # Correct distribution
        if 'Distributor ID' in self.m_lsb_release:
            vendor = self.m_lsb_release['Distributor ID']
            # Lower case and strip spaces
            vendor = string.lower(vendor)
            vendor = string.replace(vendor, " ", "")
            # DEF25168: Other special characters seem to be working
            vendor = string.replace(vendor, "/", "_")
            match = False
            for key in DISTRIBUTION_NAME_MAP:
                if vendor.startswith(key):
                    vendor = DISTRIBUTION_NAME_MAP[key]
                    match = True
                    break
            if not match:
                # Attempt to extract vendor from Description if above failed
                # (Asianux3)
                if vendor == 'n_a' or vendor == '':
                    vendor = self.m_lsb_release['Description']
                    vendor = string.lower(vendor)
                    vendor = string.replace(vendor, " ", "")
                    vendor = string.replace(vendor, "/", "_")
                    for key in DISTRIBUTION_NAME_MAP:
                        if vendor.startswith(key):
                            vendor = DISTRIBUTION_NAME_MAP[key]
                            match = True
                            break
                if not match:
                    # Create a valid distribution name from whatever we're given
                    # Endswith
                    length = len(vendor) - len("linux")
                    if length > 0 and string.rfind(vendor, "linux") == length:
                        vendor = vendor[:length]
            self.m_lsb_release['vendor'] = vendor
            self.m_vendor = vendor

        release = self.m_lsb_release.get("Release", None)
        done_os_version = False
        # Parse description for ubuntu version
        if self.m_vendor == "ubuntu":
            desc = self.m_lsb_release.get("Description", None)
            if desc is not None:
                matched = re.match(r"Ubuntu (\d+)\.(\d+)\.(\d+)", desc)
                if matched:
                    self.m_os_version = (
                        matched.group(1), matched.group(2), matched.group(3))
                    done_os_version = True
                else:
                    matched = re.match(r"Ubuntu (\d+)\.(\d+)", desc)
                    if matched:
                        self.m_os_version = (
                            matched.group(1), matched.group(2), "")
                        done_os_version = True

        # Attempt to parse the Release field into digit blocks
        if not done_os_version and release is not None:
            os_version = []
            while True:
                matched = re.match(r"(\d+)\.(.*)", release)
                if matched:
                    os_version.append(matched.group(1))
                    release = matched.group(2)
                    continue
                break
            while len(os_version) < 3:
                os_version.append("")
            self.m_os_version = os_version

    def __detect_init_process(self):
        """
        __detect_init_process
        :return:
        """
        (ret_code, stdout) = self.__back_tick(["/bin/ps", "-p1", "-ocomm="])
        if ret_code != 0:
            return None
        return stdout.strip()

    def detect_distro(self):
        """
        Detect the name of distro.
        """
        # lsb_release returned "Distribution ID"
        if 'vendor' in self.m_lsb_release:
            if 'Description' in self.m_lsb_release:
                self.product = self.m_lsb_release['Description']
            else:
                # Shouldn't really happen - LSB defines that both will be
                # returned
                self.product = 'unknown'

            return self.m_lsb_release['vendor']

        distro_check_files = ['/etc/asianux-release',
                              '/etc/slackware-version',
                              '/etc/gentoo-release',
                              '/etc/fedora-release',
                              '/etc/SuSE-release',
                              '/etc/sun-release',
                              '/etc/turbolinux-release',
                              '/etc/SLOX-release',
                              '/etc/lsb-release',
                              '/etc/oracle-release',
                              '/etc/issue',
                              '/etc/centos-release']

        distro_identified = None
        for distro_file in distro_check_files:
            distro_identified = self.check_distro_file(
                distro_file, distro_identified)

        # check some extra files if we haven't found anything yet
        if distro_identified is None:
            distro_check_files = ['/etc/redhat-release',
                                  '/etc/UnitedLinux-release',
                                  '/etc/system-release']
            for distro_file in distro_check_files:
                distro_identified = self.check_distro_file(
                    distro_file, distro_identified)

        if distro_identified is None:
            distro_identified = 'unknown'
            self.product = 'unknown'

        elif distro_identified == 'debian':
            # are we debian or knoppix?
            knoppix_ret = os.system("grep -q -s -i knoppix /etc/init")
            if knoppix_ret == 0:
                distro_identified = "knoppix"

        elif distro_identified == 'vshield':
            # are we version 1.0 or a later version?
            if os.system(
                    "grep -q -s -F 'update-package =vshield' /etc/release") != 0:
                distro_identified = "vshield-1.0"

        return distro_identified

    def check_distro_file(self, distro_file, distro_identified):
        """
        Check a file to see if it describes the distro
        """
        if not os.path.exists(distro_file):
            return distro_identified
        check_file = open(distro_file)
        original_contents = string.strip(check_file.readline())
        check_file.close()
        # Lowercase and strip spaces to match map table content
        check_contents = string.lower(original_contents)
        check_contents = string.replace(check_contents, " ", "")

        # conversion mapping between string at start of file and distro name

        for distro_string in DISTRIBUTION_NAME_MAP:
            if check_contents.startswith(distro_string):
                if distro_identified is not None:
                    if distro_identified != DISTRIBUTION_NAME_MAP[distro_string]:
                        # Ambiguous distro!
                        if self.ambiguous is None:
                            self.ambiguous = [distro_identified]
                        self.ambiguous = self.ambiguous + \
                            [DISTRIBUTION_NAME_MAP[distro_string]]
                        distro_identified = 'unknown'
                        self.product = 'unknown'
                else:
                    distro_identified = DISTRIBUTION_NAME_MAP[distro_string]
                    self.product = original_contents
        return distro_identified

    def detect_kernel(self):
        """
        Detect the name of the kernel we are running on.
        """

        if os.path.isfile('/proc/version'):
            version_file = open('/proc/version', 'r')
            version = version_file.readline()
            version_file.close()
            if version is None:
                raise self.TargetDetectionException('exc-kernel-unknown')
            detected_version = string.split(version)
            if (len(detected_version) < 3) or (detected_version[2] == ''):
                raise self.TargetDetectionException('exc-kernel-unknown')
            return detected_version[2]
        else:
            return "Unknown"

    def detect_version(self):
        """
        Detect the version of the kernel we are running on.
        """

        return self.uname[3]

    def detect_is_ec2_instance(self):
        """
        detect_is_ec2_instance
        :return:
        """
        try:
            if "ec2" in open("/sys/hypervisor/uuid").read():
                return True
        except EnvironmentError:
            pass

        try:
            data = open("/sys/devices/virtual/dmi/id/bios_version").read()
            if "amazon" in data:
                return True
        except EnvironmentError:
            pass

        try:
            data = open("/sys/devices/virtual/dmi/id/product_uuid").read()
            if data.startswith("EC2"):
                return True
        except EnvironmentError:
            pass

        try:
            results = open("/var/log/dmesg").read()
            results_list = results.split("\n")
            for item in results_list:
                line = item.lower()
                if "bios" in line and "amazon" in line:
                    return True
        except EnvironmentError:
            pass

        try:
            # Don't try two dmidecodes if the first gives an error
            devnull = open("/dev/null", "wb")
            results1 = subprocess.check_output(
                args=[
                    "dmidecode",
                    "--string",
                    "system-uuid"],
                stderr=devnull)
            results2 = subprocess.check_output(
                args=["dmidecode", "-s", "bios-version"], stderr=devnull)
            if results1.startswith("EC2") or "amazon" in results2:
                return True
        except (subprocess.CalledProcessError, EnvironmentError):
            pass

        return False

    def detect_instance_info(self, cached=True, install_dir=None):
        """
        detect_instance_info
        :return:
        """
        # Assume anything non-Linux is not on AWS
        if not self.is_linux:
            return None

        if install_dir is None:
            install_dir = self.m_install_dir

        amazon_info_path = os.path.join(install_dir, "etc", "amazon")

        if cached and os.path.isfile(amazon_info_path):
            known_aws_instance = True
        else:
            known_aws_instance = self.detect_is_ec2_instance()

        if known_aws_instance:
            try:
                proxy_handler = urllib2.ProxyHandler({})
                opener = urllib2.build_opener(proxy_handler)
                aws_info_string = opener.open(
                    "http://169.254.169.254/latest/dynamic/instance-identity/document",
                    timeout=1).read()
                aws_info = json.loads(aws_info_string)
                return {"region": aws_info["region"],
                        "accountId": aws_info["accountId"],
                        "instanceId": aws_info["instanceId"]}
            except urllib2.URLError:
                return None
        else:
            return None

    def detect_smp(self):
        """
        Detect whether or not the current kernel is SMP enabled.
        """
        if os.path.isfile('/proc/version'):
            version_file = open('/proc/version', 'r')
            version = version_file.readline()
            version_file.close()
            if version is None:
                raise self.TargetDetectionException('exc-kernel-unknown')
            return string.find(version, 'SMP') != -1
        return False

    def detect_architecture(self):
        """
        Detect on which architecture we are running on.
        """
        return self.uname[4]

    def distro_name(self):
        """
        Obtain the name distro that we are running on.
        """
        return self.distro

    def product_name(self):
        """
        Obtain the name product that we are running on.
        """
        return self.product

    def ambiguous_distro_names(self):
        """
        Provides the list of ambiguous distro identifications
        """
        return self.ambiguous

    def kernel_name(self):
        """
        Obtain the name kernel that we are running on.
        """
        return self.kernel

    def kernel_version(self):
        """
        Obtain version of the kernel that we are running on.
        """
        return self.version

    def kernel_release_version(self):
        """
        Obtain release version of the kernel that we are running on.
        """
        return self.uname[2]

    def kernel_module_suffix(self):
        """
        Returns a string containing the suffix to be used by kernel modules.
        """
        if not self.is_linux:
            return ""
        return '.ko'

    def is_smp(self):
        """
        Is the kernel multi-processor?
        """
        return self.smp

    def architecture(self):
        """
        Returns the machine architecture.
        """
        return self.arch

    def system_map(self):
        """
        system_map
        :return:
        """
        map_file = '/boot/System.map-' + self.kernel
        if os.path.isfile(map_file):
            return map_file

        map_file = '/boot/System.map'
        if os.path.isfile(map_file):
            return map_file

        path = '/boot/System.map-*-' + self.kernel
        matches = glob.glob(path)
        if len(matches) == 1 and os.path.isfile(matches[0]):
            return matches[0]

        return None

    def get_symbol_address(self, symbol):
        """
        Returns the symbol address by looking in /boot/System.map for the running kernel.
        """
        map_file = self.system_map()
        if map_file is None:
            return ''
        try:
            map_handle = open(map_file, 'r')
        except EnvironmentError:
            # Ignore permissions errors - since when it matters we will have
            # access
            return ''
        address = ''
        while True:
            line = map_handle.readline().strip()
            if line == '':
                break
            tokens = string.split(line, ' ', 3)
            if tokens[2] == symbol:
                address = '0x' + tokens[0]
                break
        map_handle.close()
        return address

    def syscall_table_address(self):
        """
        Returns the sys_call_table address for the running kernel.
        """
        return self.syscall_table

    def syscall_32_table_address(self):
        """
        Returns the ia32_sys_call_table address for the running kernel. (if applicable)
        """
        return self.syscall_32_table

    def get_platform(self):
        """
        Get the platform identifier
        """
        return self.platform

    def get_architecture(self):
        """
        Return the architecture, as needed by SAV.
        i.e.
        x86 on Linux ( AMD64)
        x86 on Solaris Intel
        sparc on Solaris Sparc
        x86 on FreeBSD
        amd on FreeBSD
        powerpc on AIX - fixed because python can't access the architecture
                       - uname -m is broken vs. other Unix
        """
        platform = self.get_platform()
        if platform == "linux":
            return "x86"
        elif platform == "sunos":
            arch = self.detect_architecture()
            if arch in ['i86pc', 'i386', 'i586']:
                return "x86"
            return "sparc"
        elif platform == "freebsd":
            arch = self.detect_architecture()
            if arch in ['i386']:
                return "x86"
            return "amd"
        elif platform == "aix":
            return "powerpc"
        return self.detect_architecture()

    def get_sub_architecture(self):
        """
        For Linux return 32 or 64 depending on architecture
        For others return ""
        """
        if self.is_linux:
            if self.arch in ["x86_64", "amd64"]:
                return "64"
            return "32"
        else:
            return ""

    def get_os_version(self):
        """
        Get the version of the operating system
        """
        if self.is_solaris:
            return self.uname[2]
        elif self.is_ubuntu:
            return self.m_lsb_release.get('Release', None)

        return None

    def version_compare(self, version_a, version_b):
        """
        Compare two version numbers, splitting on dots.
        """
        assert version_a is not None
        assert version_b is not None

        def ver(version_string):
            """
            ver
            :return:
            """
            result = []
            for part in version_string.split("."):
                try:
                    result.append(int(part))
                except ValueError:
                    result.append(0)
            return result

        vers_a = ver(version_a)
        vers_b = ver(version_b)
        return cmp(vers_a, vers_b)

    def version_is_at_least(self, other_version):
        """
        Check that the version of the OS that we are running on is at least other_version
        """
        os_version = self.get_os_version()
        if os_version is None:
            print >>sys.stderr, "WARNING: No OS Version"
            return False
        return self.version_compare(os_version, other_version) >= 0

    def kernel_version_is_at_least(self, other_version):
        """
        Check that the kernel version of the is at least other_version
        """
        kernel_short_version = self.kernel_release_version().split("-")[0]
        if kernel_short_version is None:
            print >>sys.stderr, "WARNING: No kernel Version"
            return False
        return self.version_compare(kernel_short_version, other_version) >= 0

    def glibc_version_is_at_least(self, other_version):
        """
        Check that the kernel version of the is at least other_version
        """
        glibc_version = self.get_glibc_version()
        if glibc_version is None:
            print >>sys.stderr, "WARNING: No glibc Version"
            return False
        return self.version_compare(glibc_version, other_version) >= 0

    def safe_english_locale(self, set_locale=False):
        """
        Get a safe locale for English output
        @param set_locale True = call set_locale to verify the locale is valid
        """
        if self.is_solaris and not self.version_is_at_least("5.10"):
            locale_list = [
                'en_US',
                'en_US.UTF-8',
                'en',
                'en_GB',
                'en_US.ISO8859-1',
                'C']
        else:
            locale_list = ['en_US.UTF-8',
                           'en_US',
                           'en',
                           'en_GB',
                           'en_GB.UTF-8',
                           'en_US.ISO8859-1',
                           'C']

        if set_locale:
            import locale

        for loc in locale_list:
            if set_locale:
                try:
                    locale.setlocale(locale.LC_MONETARY, loc)
                    return loc
                except locale.Error:
                    continue
            else:
                return loc
        return "C"

    def library_extension(self, name):
        """
        Return proper extension for shared library
        (or libraries if name not given) on this platform
        """
        if self.is_hpux:
            if name is None:
                return '.so'
            elif name.startswith('libsavi.'):
                return '.sl'
            return '.so'
        return '.so'

    def hostname(self):
        """
        hostname
        :return:
        """
        hostname = self.uname[1].split(".")[0]
        ten_mins = 600
        if (abs(time.time() - self.last_uname_save)
                > ten_mins or hostname == "localhost"):
            self._save_uname()
            hostname = self.uname[1].split(".")[0]
        return hostname

    def vendor(self):
        """
        vendor
        :return:
        """
        if self.m_vendor is not None:
            return self.m_vendor

        return self.distro_name()

    def os_version(self):
        """
        os_version
        :return:
        """
        return self.m_os_version

    def __detect_os_version(self):
        """
        __detect_os_version
        :return:
        """
        if self.m_os_version is not None:
            # Already detected
            return

        number_block_re = re.compile(r"\d+")
        if self.product is not None:
            blocks = number_block_re.findall(self.product)
            blocks_length = len(blocks)
            if blocks_length > 0:
                self.m_os_version = blocks
                return

        release = self.m_lsb_release.get("Release", None)
        if release is not None:
            blocks = number_block_re.findall(release)
            blocks_length = len(blocks)
            if blocks_length > 0:
                self.m_os_version = blocks
                return

        # Final fall-back - use kernel version
        (ret_code, version) = self.__back_tick(["uname", "-r"])
        if ret_code == 0:
            self.m_os_version = version.strip().split(".")
            return

        self.m_os_version = ("", "", "")

    def split_ldd_output(self, ldd_output):
        """
        split_ldd_output
        :return:
        """
        return ldd_output.splitlines()[0].split()[-1]

    def get_glibc_version(self):
        """
        get_glibc_version
        :return:
        """
        try:
            ldd = subprocess.Popen(
                ("ldd", "--version"), stdout=subprocess.PIPE)
            ldd_version = self.split_ldd_output(ldd.communicate()[0])
        except subprocess.CalledProcessError:
            pass
        except OSError:
            pass
        return ldd_version

    def minimum_sls_glibc_version(self):
        """
        minimum_sls_glibc_version
        :return:
        """
        return "2.11"

    def minimum_sls_kernel_version(self):
        """
        minimum_sls_kernel_version
        :return:
        """
        return "2.6.32"

    def check_glibc_version_for_sls(self):
        """
        check_glibc_version_for_sls
        :return:
        """
        if self.glibc_version_is_at_least(self.minimum_sls_glibc_version()):
            return True
        return False

    def check_kernel_version_for_sls(self):
        """
        check_kernel_version_for_sls
        :return:
        """
        if self.kernel_version_is_at_least(self.minimum_sls_kernel_version()):
            return True
        return False

    def check_architecture_for_sls(self):
        """
        check_architecture_for_sls
        :return:
        """
        if self.arch == "x86_64":
            return True
        return False

    def check_if_updatable_to_sls(self):
        """
        check_if_updatable_to_sls
        :return:
        """
        if self.is_linux:
            self.m_upgrade_response = []
            # Glibc - 2.11  #Kernel - 2.6.32  # AMD64 only
            if not self.check_glibc_version_for_sls():
                self.m_upgrade_response.append("Glibc version too low")

            if not self.check_kernel_version_for_sls():
                self.m_upgrade_response.append("Kernel version too low")

            if not self.check_architecture_for_sls():
                self.m_upgrade_response.append("Not supported on 32 bit")

            if not self.m_upgrade_response:
                return True

        return False

    def get_upgrade_response(self):
        """
        get_upgrade_response
        :return:
        """
        return self.m_upgrade_response

    def get_platform_guid(self):
        """
        Return the SAV (componentsuite) platform GUID
        """
        if self.is_linux:
            try:
                override_path = "/opt/sophos-svms/etc/sophos-update-product-guid"
                stat_buf = os.stat(override_path)
                if stat_buf.st_uid == 0 and stat_buf.st_gid == 0 and stat_buf.st_mode & 0o002 == 0:
                    data = open(override_path).read().strip()
                    return data
            except EnvironmentError:
                pass

            if self.distro == "vshield":
                return "5B6B70A6-3C03-47EA-81D2-60C38376135F"
            return "5CF594B0-9FED-4212-BA91-A4077CB1D1F3"
        elif self.is_solaris:
            if self.get_architecture() == "sparc":
                return "5CFA73B4-DBC2-4A70-8B18-C29AA9014C81"
            return "CF87F829-EA39-4D74-97F7-B849606F0044"
        elif self.is_hpux:
            return "905F6B56-DF17-4CF8-B413-7162F2D2DD3F"
        elif self.is_aix:
            return "43F3AB41-6B33-40A7-8E99-22F237C97AB3"

        raise Exception("Not a supported platform")


def printout(stream):
    """
    printout
    :return:
    """
    target_system = TargetSystem()
    stream.write("Platform:            %s\n" % (target_system.platform))
    stream.write("Distro Name:         " +
                 str(target_system.distro_name()) + "\n")
    product_name = target_system.product_name()
    stream.write("Product Name:        " + str(product_name) + "\n")
    stream.write("Kernel Name:         " +
                 str(target_system.kernel_name()) + "\n")
    stream.write("Module suffix:       " +
                 str(target_system.kernel_module_suffix()) + "\n")
    stream.write("is_smp:               " + str(target_system.is_smp()) + "\n")
    stream.write("Architecture:        " +
                 str(target_system.architecture()) + "\n")
    stream.write("Version:             " +
                 str(target_system.kernel_version()) + "\n")
    stream.write("system_map:           " +
                 str(target_system.system_map()) + "\n")
    stream.write("syscall_table:        " +
                 str(target_system.syscall_table_address()) + "\n")
    stream.write(
        "PlatformGUID:        %s\n" %
        (target_system.get_platform_guid()))
    if target_system.architecture() == 'x86_64':
        stream.write("IA32 syscall_table:   " +
                     str(target_system.syscall_32_table_address()) + "\n")
    if target_system.is_linux:
        stream.write("Vendor:              %s\n" % (target_system.vendor()))
        stream.write("is_ubuntu:            %s\n" %
                     (str(target_system.is_ubuntu)))
        stream.write("is_redhat:            %s\n" %
                     (str(target_system.is_redhat)))
        stream.write("use upstart:         %s\n" %
                     (str(target_system.use_upstart)))
        stream.write("use systemd:         %s\n" %
                     (str(target_system.use_systemd)))
        stream.write("has upstart:         %s\n" %
                     (str(target_system.has_upstart)))
        stream.write("has systemd:         %s\n" %
                     (str(target_system.has_systemd)))
        stream.write("using lsb_release:   %s\n" %
                     (str(target_system.using_lsb_release())))

    if target_system.ambiguous_distro_names() is not None:
        stream.write(str(target_system.ambiguous_distro_names()) + "\n")
    return (product_name, target_system)


def main():
    """
    main
    :return:
    """
    printout(sys.stdout)
    return 0


if __name__ == "__main__":
    sys.exit(main())

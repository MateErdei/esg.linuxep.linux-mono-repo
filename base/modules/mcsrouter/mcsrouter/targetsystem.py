# -*- coding: utf-8 -*-

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
    'debian'                                : 'debian',
    'gentoo'                                : 'gentoo',
    'mandrake'                              : 'mandrake',
    'redhat'                                : 'redhat',
    'slackware'                             : 'slackware',
    'suse'                                  : 'suse',
    'opensuse'                              : 'suse',
    'sunjavadesktop'                        : 'sjd',
    'turbolinux'                            : 'turbo',
    'unitedlinux'                           : 'suse',
    'fedora'                                : 'fedora',
    'ubuntu'                                : 'ubuntu',
    'asianux'                               : 'asianux',
    'oracle'                                : 'oracle',
    'centos'                                : 'centos',
    'amazonlinux'                           : 'amazon',

    # Sophos Anti-Virus for VMWare platforms.
    # NB, there is also a vshield-1.0 which can be assigned by detectDistro
    # and which corresponds to the "special" 1.0 release of SAV for vShield which
    # didn't have a dedicated update location.
    'sophosanti-virusforvmwarevshield'      : 'vshield',
}

#-----------------------------------------------------------------------------
# class TargetSystem
#-----------------------------------------------------------------------------
class TargetSystem(object):
    '''
    Represents the system into which we are trying to install/load TALPA.
    '''

    class TargetDetectionException(Exception):
        pass

    def __init__(self, installDir="."):
        self._saveUname()
        self.platform = self.uname[0].lower().replace("-","")
        self.isLinux = (self.platform == "linux")
        self.isSolaris = (self.platform == "sunos")
        self.isHPUX = (self.platform == "hpux")
        self.isFreeBSD = (self.platform == "freebsd")
        self.isAIX = (self.platform == "aix")
        self.m_vendor = None
        self.m_osVersion = None
        self.m_upgradeResponse = []
        self.m_installDir = installDir

        ## First check if lsb_release is available
        ## Avoid checking for oracle since this generated a very long binary pack name, use oracle-release instead
        self.__collectLsbRelease()

        self.ambiguous          = None
        self.product            = None
        self.distro             = self.detectDistro()
        self.kernel             = self.detectKernel()
        self.smp                = self.detectSMP()
        self.arch               = self.detectArchitecture()
        self.version            = self.detectVersion()
        self.awsInstanceInfo    = self.detectInstanceInfo(installDir=installDir)
        self.syscallTable       = self.getSymbolAddress('sys_call_table')
        if self.arch == 'x86_64':
            self.syscall32Table = self.getSymbolAddress('ia32_sys_call_table')
        else:
            self.syscall32Table = ''

        self.isUbuntu = (self.distro == "ubuntu")
        self.isRedhat = (self.distro == "redhat")

        # we only use upstart on Ubuntu
        self.configureUpstart = self.isUbuntu and self.versionIsAtLeast("10.04") and os.path.isdir("/etc/init")
        self.configureSystemd = os.path.isdir("/usr/lib/systemd/system") or os.path.isdir("/lib/systemd/system")

        self.hasUpstart = self.configureUpstart and os.path.isfile("/sbin/initctl")
        self.hasSystemd = self.configureSystemd and os.path.isfile("/bin/systemctl")

        # Ubuntu 15.04 (and 14.10) can have *both* upstart and systemd installed concurrently.
        # We need to detect the active mechanism.
        if self.hasUpstart and self.hasSystemd:
            initProcess = self.__detectInitProcess()
            self.useSystemd = (initProcess == "systemd")
            self.useUpstart = (initProcess == "init" or initProcess == "upstart")
        else:
            self.useUpstart = self.hasUpstart
            self.useSystemd = self.hasSystemd

        self.useSMF = self.isSolaris and self.versionIsAtLeast("5.10")

        ## True if daemons are started asynchronously, so longer waits are required for savd to activate
        self.asyncStart = self.useUpstart or self.useSystemd

        self.osName = self.product
        OS_NAME_RELEASE_RE = re.compile(r" release ",re.I)
        self.osName = OS_NAME_RELEASE_RE.sub(" ",self.osName)  ## Make the name shorter if possible

        self.__detectOsVersion()

    def _readUname(self):
        try:
            return os.uname()
        except OSError: # os.uname() can fail on HP-UX with > 8 character hostnames
            return socket.gethostname()

    def _saveUname(self):
        self.uname = self._readUname()
        self.lastUnameSave = time.time()

    def __backtick(self,command):
        if isinstance(command,basestring):
            command = [command]
        ## Clean environment
        env = os.environ.copy()

        ## Need to reset the python variables in case command is a python script

        def resetEnv(env,variable):
            orig = "ORIGINAL_%s"%variable
            if env.has_key(orig) and env[orig] != "":
                env[variable] = env[orig]
            else:
                env.pop(variable,None) ## Ignore the variable if it's missing

        def removeEnvVariable(env,variable):
            env.pop(variable,None)

        resetEnv(env,'PYTHONPATH')
        resetEnv(env,'LD_LIBRARY_PATH')
        resetEnv(env,'PYTHONHOME')


        def attempt(command,env,shell=False):
            try:
                proc = subprocess.Popen(command, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                    shell=shell)
                (stdout,stderr) = proc.communicate()
                retCode = proc.wait()
                assert retCode is not None
            except OSError:
                retCode = -1
                stdout = ""
            return (retCode,stdout)

        (retCode,stdout) = attempt(command,env)
        if retCode == 0:
            return (retCode,stdout)

        ## Maybe we tried to execute a Python program with incorrect environment
        ## so lets try again without any.
        clearEnv = env.copy()
        removeEnvVariable(clearEnv,'PYTHONPATH')
        removeEnvVariable(clearEnv,'LD_LIBRARY_PATH')
        removeEnvVariable(clearEnv,'PYTHONHOME')

        (retCode,stdout) = attempt(command,clearEnv)
        if retCode == 0:
            return (retCode,stdout)

        ## Try via the shell

        (retCode,stdout) = attempt(command,env,shell=True)
        if retCode == 0:
            return (retCode,stdout)
        (retCode,stdout) = attempt(command,clearEnv,shell=True)
        if retCode == 0:
            return (retCode,stdout)

        ## Try with restricted PATH
        env['PATH'] = "/usr/bin:/bin:/usr/sbin:/sbin"
        (retCode,stdout) = attempt(command,env)
        if retCode == 0:
            return (retCode,stdout)
        clearEnv['PATH'] = env['PATH']
        (retCode,stdout) = attempt(command,clearEnv)
        if retCode == 0:
            return (retCode,stdout)

        return (retCode,stdout)

    def using_lsb_release(self):
        return len(self.m_lsb_release) > 0

    def __collectLsbRelease(self):
        """
        Collect any lsb_release info
        """
        self.m_lsb_release = {}

        if os.path.isfile('/etc/oracle-release'):
            return
        PATH=os.environ.get("PATH","")
        lsb_release_exes = []
        for p in PATH.split(":"):
            lsb_release = os.path.join(p,"lsb_release")
            if os.path.isfile(lsb_release):
                lsb_release_exes.append(lsb_release)

        if "/usr/bin/lsb_release" not in lsb_release_exes and os.path.isfile("/usr/bin/lsb_release"):
            lsb_release_exes.append("/usr/bin/lsb_release")

        retCode = -2
        for lsb_release_exe in lsb_release_exes:
            (retCode,stdout) = self.__backtick([lsb_release_exe,'-a'])
            if retCode == 0:
                break

        if retCode != 0:
            ## failed to run lsb_release
            return

        lines = stdout.split("\n")
        ## Each line consists of "Key:<whitespace>*<value>"
        import re
        matcher = re.compile(r"([^:]+):\s*(.+)$")
        for l in lines:
            mo = matcher.match(l)
            if mo:
                self.m_lsb_release[mo.group(1)] = mo.group(2)

        ## Correct distribution
        if self.m_lsb_release.has_key('Distributor ID'):
            vendor = self.m_lsb_release['Distributor ID']
            ## Lower case and strip spaces
            vendor = string.lower(vendor)
            vendor = string.replace(vendor," ","")
            vendor = string.replace(vendor,"/","_") # DEF25168: Other special characters seem to be working
            match = False
            for key in DISTRIBUTION_NAME_MAP.keys():
                if vendor.startswith(key):
                    vendor = DISTRIBUTION_NAME_MAP[key]
                    match = True
                    break
            if not match:
                ## Attempt to extract vendor from Description if above failed (Asianux3)
                if vendor == 'n_a' or vendor == '':
                    vendor = self.m_lsb_release['Description']
                    vendor = string.lower(vendor)
                    vendor = string.replace(vendor," ","")
                    vendor = string.replace(vendor,"/","_")
                    for key in DISTRIBUTION_NAME_MAP.keys():
                        if vendor.startswith(key):
                            vendor = DISTRIBUTION_NAME_MAP[key]
                            match = True
                            break
                if not match:
                    ## Create a valid distribution name from whatever we're given
                    ## Endswith
                    length = len(vendor) - len("linux")
                    if length > 0 and string.rfind(vendor,"linux") == length:
                        vendor = vendor[:length]
            self.m_lsb_release['vendor'] = vendor
            self.m_vendor = vendor

        release = self.m_lsb_release.get("Release",None)
        doneOSVersion = False
        ## Parse description for ubuntu version
        if self.m_vendor == "ubuntu":
            desc = self.m_lsb_release.get("Description",None)
            if desc is not None:
                mo = re.match(r"Ubuntu (\d+)\.(\d+)\.(\d+)",desc)
                if mo:
                    self.m_osVersion = (mo.group(1),mo.group(2),mo.group(3))
                    doneOSVersion = True
                else:
                    mo = re.match(r"Ubuntu (\d+)\.(\d+)",desc)
                    if mo:
                        self.m_osVersion = (mo.group(1),mo.group(2),"")
                        doneOSVersion = True

        ## Attempt to parse the Release field into digit blocks
        if not doneOSVersion and release is not None:
            osVersion = []
            while True:
                mo = re.match(r"(\d+)\.(.*)",release)
                if mo:
                    osVersion.append(mo.group(1))
                    release = mo.group(2)
                    continue
                break
            while len(osVersion) < 3:
                osVersion.append("")
            self.m_osVersion = osVersion


    def __detectInitProcess(self):
        ## TODO: implement in native python?
        (retCode,stdout) = self.__backtick(["/bin/ps","-p1","-ocomm="])
        if retCode != 0:
            return None
        return stdout.strip()

    def detectDistro(self):
        '''
        Detect the name of distro.
        '''
        ## lsb_release returned "Distribution ID"
        if self.m_lsb_release.has_key('vendor'):
            if self.m_lsb_release.has_key('Description'):
                self.product = self.m_lsb_release['Description']
            else:
                ## Shouldn't really happen - LSB defines that both will be returned
                self.product = 'unknown'

            return self.m_lsb_release['vendor']

        distroCheckFiles = [
                            '/etc/asianux-release',
                            '/etc/slackware-version',
                            '/etc/gentoo-release',
                            '/etc/fedora-release',
                            '/etc/sun-release',
                            '/etc/SuSE-release',
                            '/etc/turbolinux-release',
                            '/etc/SLOX-release',
                            '/etc/lsb-release',
                            '/etc/oracle-release',
                            '/etc/issue',
                            '/etc/centos-release'
                          ]

        distroIdentified = None
        for distroFile in distroCheckFiles:
            distroIdentified = self.checkDistroFile(distroFile,distroIdentified)

        # check some extra files if we haven't found anything yet
        if distroIdentified is None:
            distroCheckFiles = [
                                '/etc/redhat-release',
                                '/etc/UnitedLinux-release',
                                '/etc/system-release'
                               ]
            for distroFile in distroCheckFiles:
                distroIdentified = self.checkDistroFile(distroFile,distroIdentified)

        if distroIdentified is None:
            distroIdentified = 'unknown'
            self.product='unknown'

        elif distroIdentified == 'debian':
            # are we debian or knoppix?
            knoppixRet = os.system("grep -q -s -i knoppix /etc/init")
            if knoppixRet == 0:
                distroIdentified = "knoppix"

        elif distroIdentified == 'vshield':
            # are we version 1.0 or a later version?
            if os.system("grep -q -s -F 'update-package=vshield' /etc/release") != 0:
                distroIdentified = "vshield-1.0"

        return distroIdentified

    def checkDistroFile(self,distroFile,distroIdentified):
        """Check a file to see if it describes the distro"""

        if not os.path.exists(distroFile):
            return distroIdentified
        checkFile = open(distroFile)
        originalContents = string.strip(checkFile.readline())
        checkFile.close()
        # Lowercase and strip spaces to match map table content
        checkContents = string.lower(originalContents)
        checkContents = string.replace(checkContents," ","")

        # conversion mapping between string at start of file and distro name

        for distroString in DISTRIBUTION_NAME_MAP.keys():
            if checkContents.startswith(distroString):
                if distroIdentified is not None:
                    if distroIdentified != DISTRIBUTION_NAME_MAP[distroString]:
                        # Ambiguous distro!
                        if self.ambiguous is None:
                            self.ambiguous = [ distroIdentified ]
                        self.ambiguous = self.ambiguous + [ DISTRIBUTION_NAME_MAP[distroString] ]
                        distroIdentified = 'unknown'
                        self.product = 'unknown'
                else:
                    distroIdentified = DISTRIBUTION_NAME_MAP[distroString]
                    self.product = originalContents
        return distroIdentified


    def detectKernel(self):
        '''
        Detect the name of the kernel we are running on.
        '''

        ## TODO - Solaris/Unix detection
        if os.path.isfile('/proc/version'):
            versionFile = open('/proc/version', 'r')
            version = versionFile.readline()
            versionFile.close()
            if version == None:
                raise TargetDetectionException('exc-kernel-unknown')
            detectedVersion = string.split(version)
            if (len(detectedVersion) < 3) or (detectedVersion[2] == ''):
                raise TargetDetectionException('exc-kernel-unknown')
            return detectedVersion[2]
        else:
            return "Unknown"

    def detectVersion(self):
        '''
        Detect the version of the kernel we are running on.
        '''

        return self.uname[3]

    def detectIsEC2Instance(self):
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
            ## Don't try two dmidecodes if the first gives an error
            devnull = open("/dev/null","wb")
            results = subprocess.check_output(args=["dmidecode", "--string", "system-uuid"],stderr=devnull)
            if results.startswith("EC2"):
                return True
            results = subprocess.check_output(args=["dmidecode", "-s", "bios-version"],stderr=devnull)
            if "amazon" in results:
                return True
        except (subprocess.CalledProcessError,EnvironmentError):
            pass

        return False

    def detectInstanceInfo(self, cached=True, installDir=None):
        ## Assume anything non-Linux is not on AWS
        if not self.isLinux:
            return None

        if installDir is None:
            installDir = self.m_installDir

        amazon_info_path = os.path.join(installDir,"etc","amazon")

        if cached and os.path.isfile(amazon_info_path):
            known_aws_instance = True
        else:
            known_aws_instance = self.detectIsEC2Instance()

        if known_aws_instance:
            try:
                proxy_handler = urllib2.ProxyHandler({})
                opener = urllib2.build_opener(proxy_handler)
                aws_info_string = opener.open("http://169.254.169.254/latest/dynamic/instance-identity/document", timeout = 1).read()
                aws_info = json.loads(aws_info_string)
                return {"region" : aws_info["region"],
                        "accountId" : aws_info["accountId"],
                        "instanceId" : aws_info["instanceId"]
                        }
            except Exception:
                return None
        else:
            return None

    def detectSMP(self):
        '''
        Detect whether or not the current kernel is SMP enabled.
        '''
        ## TODO - Solaris/Unix detection
        if os.path.isfile('/proc/version'):
            versionFile = open('/proc/version', 'r')
            version = versionFile.readline()
            versionFile.close()
            if version is None:
                raise TargetDetectionException('exc-kernel-unknown')
            return string.find(version,'SMP') != -1
        return False

    def detectArchitecture(self):
        '''
        Detect on which architecture we are running on.
        '''
        return self.uname[4]

    def distroName(self):
        '''
        Obtain the name distro that we are running on.
        '''
        return self.distro

    def productName(self):
        '''
        Obtain the name product that we are running on.
        '''
        return self.product

    def ambiguousDistroNames(self):
        '''
        Provides the list of ambiguous distro identifications
        '''
        return self.ambiguous

    def kernelName(self):
        '''
        Obtain the name kernel that we are running on.
        '''
        return self.kernel

    def kernelVersion(self):
        '''
        Obtain version of the kernel that we are running on.
        '''
        return self.version

    def kernelReleaseVersion(self):
        '''
        Obtain release version of the kernel that we are running on.
        '''
        return self.uname[2]


    def kernelModuleSuffix(self):
        '''
        Returns a string containing the suffix to be used by kernel modules.
        '''
        ## TODO: If we add kernel modules on other distributions
        if not self.isLinux:
            return ""
        return '.ko'

    def isSMP(self):
        '''
        Is the kernel multi-processor?
        '''
        return self.smp

    def architecture(self):
        '''
        Returns the machine architecture.
        '''
        return self.arch

    def systemMap(self):
        mapFile = '/boot/System.map-' + self.kernel
        if os.path.isfile(mapFile):
            return mapFile

        mapFile = '/boot/System.map'
        if os.path.isfile(mapFile):
            return mapFile

        g = '/boot/System.map-*-' + self.kernel
        matches = glob.glob(g)
        if len(matches) == 1 and os.path.isfile(matches[0]):
            return matches[0]

        return None


    def getSymbolAddress(self,symbol):
        '''
        Returns the symbol address by looking in /boot/System.map for the running kernel.
        '''
        mapFile = self.systemMap()
        if mapFile is None:
            return ''
        try:
            mapHandle = open(mapFile, 'r')
        except EnvironmentError:
            ## Ignore permissions errors - since when it matters we will have access
            return ''
        address = ''
        while True:
            line = mapHandle.readline().strip()
            if line == '':
                    break
            tokens = string.split(line, ' ', 3)
            if tokens[2] == symbol:
                address = '0x' + tokens[0]
                break
        mapHandle.close()
        return address

    def syscallTableAddress(self):
        '''
        Returns the sys_call_table address for the running kernel.
        '''
        return self.syscallTable

    def syscall32TableAddress(self):
        '''
        Returns the ia32_sys_call_table address for the running kernel. (if applicable)
        '''
        return self.syscall32Table

    def getPlatform(self):
        """
        Get the platform identifier
        """
        return self.platform

    def getArchitecture(self):
        """
        Return the architecture, as needed by SAV.
        i.e.
        x86 on Linux (TODO: AMD64)
        x86 on Solaris Intel
        sparc on Solaris Sparc
        x86 on FreeBSD
        amd on FreeBSD
        powerpc on AIX - fixed because python can't access the architecture - uname -m is broken vs. other Unix
        """
        platform = self.getPlatform()
        if platform == "linux":
            return "x86"
        elif platform == "sunos":
            arch = self.detectArchitecture()
            if arch in ['i86pc','i386','i586']:
                return "x86"
            else:
                return "sparc"
        elif platform == "freebsd":
            arch = self.detectArchitecture()
            if arch in ['i386']:
                return "x86"
            else:
                # TODO: amd_64? x86_64?
                return "amd"
        elif platform == "aix":
            return "powerpc"
        else:
            return self.detectArchitecture()

    def getSubArchitecture(self):
        """
        For Linux return 32 or 64 depending on architecture
        For others return ""
        """
        if self.isLinux:
            if self.arch in ["x86_64","amd64"]:
                return "64"
            return "32"
        else:
            return ""

    def getOSVersion(self):
        """
        Get the version of the operating system
        """
        if self.isSolaris:
            return self.uname[2]
        elif self.isUbuntu:
            return self.m_lsb_release.get('Release',None)

        return None

    def versionCompare(self, versionA, versionB):
        """
        Compare two version numbers, splitting on dots.
        """
        assert versionA is not None
        assert versionB is not None

        def ver(s):
            r = []
            for part in s.split("."):
                try:
                    r.append(int(part))
                except ValueError:
                    r.append(0)
            return r

        a = ver(versionA)
        b = ver(versionB)
        return cmp(a,b)

    def versionIsAtLeast(self, otherVersion):
        """
        Check that the version of the OS that we are running on is at least otherVersion
        """
        osversion = self.getOSVersion()
        if osversion is None:
            print >>sys.stderr,"WARNING: No OS Version"
            return False
        return self.versionCompare(osversion,otherVersion) >= 0

    def kernelVersionIsAtLeast(self, otherVersion):
        """
        Check that the kernel version of the is at least otherVersion
        """
        kernelShortVersion = self.kernelReleaseVersion().split("-")[0]
        if kernelShortVersion is None:
            print >>sys.stderr,"WARNING: No kernel Version"
            return False
        return self.versionCompare(kernelShortVersion,otherVersion) >= 0

    def glibcVersionIsAtLeast(self, otherVersion):
        """
        Check that the kernel version of the is at least otherVersion
        """
        glibcVersion = self.getGlibcVersion()
        if glibcVersion is None:
            print >>sys.stderr,"WARNING: No glibc Version"
            return False
        return self.versionCompare(glibcVersion,otherVersion) >= 0

    def safeEnglishLocale(self, setlocale=False):
        """
        Get a safe locale for English output
        @param setlocale True = call setlocale to verify the locale is valid
        """
        if self.isSolaris and not self.versionIsAtLeast("5.10"):
            l = ['en_US','en_US.UTF-8','en','en_GB','en_US.ISO8859-1','C']
        else:
            l = ['en_US.UTF-8','en_US','en','en_GB','en_GB.UTF-8','en_US.ISO8859-1','C']

        if setlocale:
            import locale

        for loc in l:
            if setlocale:
                try:
                    locale.setlocale(locale.LC_MONETARY,loc)
                    return loc
                except locale.Error:
                    continue
            else:
                return loc
        return "C"

    def libraryExtension(self, name):
        """
        Return proper extension for shared library (or libraries if name is not given) on this platform
        """
        if self.isHPUX:
            if name == None:
                return '.so'
            elif name.startswith('libsavi.'):
                return '.sl'
            else:
                return '.so'
        else:
            return '.so'

    def hostname(self):
        hostname = self.uname[1].split(".")[0]
        tenMins = 600
        if (abs(time.time() - self.lastUnameSave) > tenMins or hostname == "localhost"):
            self._saveUname()
            hostname = self.uname[1].split(".")[0]
        return hostname

    def vendor(self):
        if self.m_vendor is not None:
            return self.m_vendor

        return self.distroName()

    def osVersion(self):
        return self.m_osVersion

    def __detectOsVersion(self):
        if self.m_osVersion is not None:
            ## Already detected
            return

        NUMBER_BLOCK_RE = re.compile(r"\d+")
        if self.product is not None:
            blocks = NUMBER_BLOCK_RE.findall(self.product)
            if len(blocks) > 0:
                self.m_osVersion = blocks
                return

        release = self.m_lsb_release.get("Release",None)
        if release is not None:
            blocks = NUMBER_BLOCK_RE.findall(release)
            if len(blocks) > 0:
                self.m_osVersion = blocks
                return

        ## Final fall-back - use kernel version
        (retCode,version) = self.__backtick(["uname", "-r"])
        if retCode == 0:
            self.m_osVersion = version.strip().split(".")
            return

        self.m_osVersion = ("","","")

    def splitLddOutput(self, lddOutput):
        return lddOutput.splitlines()[0].split()[-1]

    def getGlibcVersion(self):
        try:
            ldd = subprocess.Popen(("ldd", "--version"), stdout=subprocess.PIPE)
            lddVersion = self.splitLddOutput(ldd.communicate()[0])
        except subprocess.CalledProcessError:
            pass
        except OSError:
            pass
        return lddVersion

    def minimumSLSGlibcVersion(self):
        return "2.11"

    def minimumSLSKernelVersion(self):
        return "2.6.32"

    def checkGlibcVersionForSLS(self):
        if self.glibcVersionIsAtLeast(self.minimumSLSGlibcVersion()):
            return True
        return False

    def checkKernelVersionForSLS(self):
        if self.kernelVersionIsAtLeast(self.minimumSLSKernelVersion()):
            return True
        return False

    def checkArchitectureForSLS(self):
        if self.arch == "x86_64":
            return True
        return False

    def checkIfUpdatableToSLS(self):
        if self.isLinux:
            self.m_upgradeResponse = []
            # Glibc - 2.11  #Kernel - 2.6.32  # AMD64 only
            if not self.checkGlibcVersionForSLS():
                self.m_upgradeResponse.append("Glibc version too low")

            if not self.checkKernelVersionForSLS():
                self.m_upgradeResponse.append("Kernel version too low")

            if not self.checkArchitectureForSLS():
                self.m_upgradeResponse.append("Not supported on 32 bit")

            if not self.m_upgradeResponse:
                return True

        return False

    def getUpgradeResponse(self):
        return self.m_upgradeResponse

    def getPlatformGUID(self):
        """
        Return the SAV (componentsuite) platform GUID
        """
        if self.isLinux:
            try:
                overridePath = "/opt/sophos-svms/etc/sophos-update-product-guid"
                statbuf = os.stat(overridePath)
                if statbuf.st_uid == 0 and statbuf.st_gid == 0 and statbuf.st_mode & 0o002 == 0:
                    data = open(overridePath).read().strip()
                    return data
            except EnvironmentError:
                pass

            if self.distro == "vshield":
                return "5B6B70A6-3C03-47EA-81D2-60C38376135F"
            else:
                return "5CF594B0-9FED-4212-BA91-A4077CB1D1F3"
        elif self.isSolaris:
            if self.getArchitecture() == "sparc":
                return "5CFA73B4-DBC2-4A70-8B18-C29AA9014C81"
            else:
                return "CF87F829-EA39-4D74-97F7-B849606F0044"
        elif self.isHPUX:
            return "905F6B56-DF17-4CF8-B413-7162F2D2DD3F"
        elif self.isAIX:
            return "43F3AB41-6B33-40A7-8E99-22F237C97AB3"

        raise Exception("Not a supported platform")

def printout(stream):
    t = TargetSystem()
    stream.write("Platform:            %s\n"%(t.platform))
    stream.write("Distro Name:         "+str(t.distroName())+"\n")
    productName = t.productName()
    stream.write("Product Name:        "+str(productName)+"\n")
    stream.write("Kernel Name:         "+str(t.kernelName())+"\n")
    stream.write("Module suffix:       "+str(t.kernelModuleSuffix())+"\n")
    stream.write("IsSMP:               "+str(t.isSMP())+"\n")
    stream.write("Architecture:        "+str(t.architecture())+"\n")
    stream.write("Version:             "+str(t.kernelVersion())+"\n")
    stream.write("SystemMap:           "+str(t.systemMap())+"\n")
    stream.write("SyscallTable:        "+str(t.syscallTableAddress())+"\n")
    stream.write("PlatformGUID:        %s\n"%(t.getPlatformGUID()))
    if t.architecture() == 'x86_64':
        stream.write("IA32 SyscallTable:   "+str(t.syscall32TableAddress())+"\n")
    if t.isLinux:
        stream.write("Vendor:              %s\n"%(t.vendor()))
        stream.write("isUbuntu:            %s\n"%(str(t.isUbuntu)))
        stream.write("isRedhat:            %s\n"%(str(t.isRedhat)))
        stream.write("use upstart:         %s\n"%(str(t.useUpstart)))
        stream.write("use systemd:         %s\n"%(str(t.useSystemd)))
        stream.write("has upstart:         %s\n"%(str(t.hasUpstart)))
        stream.write("has systemd:         %s\n"%(str(t.hasSystemd)))
        stream.write("using lsb_release:   %s\n"%(str(t.using_lsb_release())))

    if t.ambiguousDistroNames() != None:
        stream.write(str(t.ambiguousDistroNames())+"\n")
    return (productName,t)


def main(argv):
    printout(sys.stdout)
    return 0

if __name__ == "__main__":
    sys.exit(main(sys.argv))

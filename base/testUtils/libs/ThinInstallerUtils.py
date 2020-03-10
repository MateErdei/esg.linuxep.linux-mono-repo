import errno
import stat
import glob
import subprocess
import os
import shutil
import robot.api.logger as logger
import robot
import hashlib
import re
import xml.dom.minidom

import WarehouseUtils
import CentralUtils
from robot.libraries.BuiltIn import BuiltIn

import PathManager


def extract_hashed_credentials_from_alc_policy(alc_file_path):
    if not os.path.exists(alc_file_path):
        raise AssertionError("File does not exist {}".format(alc_file_path))
    doc = xml.dom.minidom.parseString(open(alc_file_path, 'r').read())
    primary = doc.getElementsByTagName('primary_location')[0]
    server = primary.getElementsByTagName('server')[0]
    username=server.getAttribute('UserName')
    password=server.getAttribute('UserPassword')
    connection=server.getAttribute('ConnectionAddress')
    algorithm = server.getAttribute('Algorithm')
    if algorithm != 'Clear':
        raise AssertionError("Only clear algorithm is supported right now. Given Algorithm: {}".format(algorithm))
    return  WarehouseUtils.calculate_hashed_creds(username, password), connection


class ThinInstallerUtils(object):
    def __init__(self):

        # Setup a temp dir.
        self.tmp_path = os.path.join(".", "tmp", "thin_installer")
        self.cleanup_files()
        os.makedirs(self.tmp_path)

        self.log_path = os.path.join(self.tmp_path, "ThinInstaller.log")
        self.installer_files = os.path.join(self.tmp_path, "thininstaller_files/")
        self.default_installsh_path = os.path.join(self.installer_files, "installer-default.sh")
        self.install_process = None
        self.env = os.environ.copy()
        self.original_path = os.environ['PATH']
        filer6 = "/mnt/filer6/bfr"
        if not os.path.isdir(os.path.join(filer6, "sspl-thininstaller")):
            filer6 = "/uk-filer6/bfr"

        self.last_good_artisan_build_file = os.path.join(filer6,
                                                         "sspl-thininstaller",
                                                         "master/sspl-thininstaller_lastgoodbuild.txt")
        self.https_certs_dir = os.path.join(PathManager.get_support_file_path(), "https/ca")

        try:
            os.makedirs(self.installer_files)
        except OSError as ex:
            if ex.errno != errno.EEXIST:
                raise

        self.default_credentials_file_location = os.path.join(self.installer_files, "credentials.txt")

    def dump_log(self):
        filename = self.log_path
        if os.path.exists(filename):
            try:
                with open(filename, "r") as f:
                    logger.info("Log output (from %s): " % filename+''.join(f.readlines()))
            except Exception as e:
                logger.info("Failed to read file {}: {}".format(filename, e))
        else:
            logger.info("File %s does not exist" % filename)

    def teardown_reset_original_path(self):
        self.env["PATH"] = self.original_path

    def cleanup_files(self):
        if os.path.isdir(self.tmp_path):
            shutil.rmtree(self.tmp_path)
        if os.path.lexists("/tmp/savscan"):
            os.unlink("/tmp/savscan")
        if os.path.lexists("/tmp/sweep"):
            os.unlink("/tmp/sweep")

    def find_thininstaller_output(self, source=None):
        source = os.environ.get("THIN_INSTALLER_OVERRIDE", source)
        if source is not None:
            logger.info("using {} as source".format(source))
            return source

        local_dir = "/vagrant/thininstaller/output"
        if os.path.isdir(local_dir):
            logger.info("Thin Installer source: " + local_dir)
            return local_dir

        if os.path.isfile(self.last_good_artisan_build_file):

            with open(self.last_good_artisan_build_file) as f:
                last_good_build = f.read()

            print("Last good Thin Installer build: {}".format(last_good_build))
            source_folder = os.path.join("/mnt", "filer6", "bfr", "sspl-thininstaller", "master", last_good_build, "sspl-thininstaller")
            version_dirs = os.listdir(source_folder)
            if len(version_dirs) == 1:
                source = os.path.join(source_folder, version_dirs[0], "output")
            else:
                raise AssertionError("More than one version in the build for thininstaller: {}".format(version_dirs))
            logger.info("using {} as source".format(source))
            return source

        raise AssertionError("Could not find thininstaller output")

    def get_thininstaller(self, source=None):
        source = self.find_thininstaller_output(source)
        self.copy_and_unpack_thininstaller(source)

    def copy_and_unpack_thininstaller(self, source):
        print("Getting Thin Installer from: {}".format(source))
        shutil.rmtree(self.installer_files)
        shutil.copytree(source, self.installer_files)

        list_of_files = glob.glob(os.path.join(self.installer_files, "*.tar.gz"))
        if len(list_of_files) == 0:
            raise AssertionError("Could not find a thin installer .tar.gz file!")

        installer_tar_gz = max(list_of_files, key=os.path.getctime)
        os.system("tar xzf {} -C {}".format(installer_tar_gz, self.installer_files))

    def create_credentials_file(self, token, url, update_creds, message_relays, location, update_caches):
        new_line = '\n'
        with open(location, 'w') as fh:
            fh.write("TOKEN=" + token + new_line)
            fh.write("URL=" + url + new_line)
            fh.write("UPDATE_CREDENTIALS=" + update_creds + new_line)
            if message_relays:
                fh.write("MESSAGE_RELAYS=" + message_relays + new_line)
            if update_caches:
                fh.write("UPDATE_CACHES=" + update_caches + new_line)
                fh.write("__UPDATE_CACHE_CERTS__" + new_line)
                with open(os.path.join(self.https_certs_dir, "root-ca.crt.pem"), "r") as f:
                    fh.write(f.read())
        with open(location, 'r') as fh:
            robot.api.logger.info(fh.read())

    def create_default_credentials_file(self,
                                        token="ThisIsARegToken",
                                        url="https://localhost:4443/mcs",
                                        update_creds="9539d7d1f36a71bbac1259db9e868231",
                                        message_relays=None,
                                        update_caches=None):
        self.create_credentials_file(token, url, update_creds, message_relays, self.default_credentials_file_location, update_caches)

    def build_thininstaller_from_sections(self, credentials_file, target_path=None):
        if target_path is None:
            target_path = self.default_installsh_path
        command = "sed '/^__MIDDLE_BIT__/r {}' {} > {}".format(credentials_file, os.path.join(self.installer_files, "SophosSetup.sh"), target_path)
        process = subprocess.Popen(command, shell=True, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
        stdout, stderr = process.communicate()
        rc = process.returncode
        logger.info("stdout:\n {}".format(stdout))
        logger.info("stderr:\n {}".format(stderr))


        if rc != 0:
            raise AssertionError("Could not run sed replacement of __MIDDLE_BIT__ with credentials. Command = {}".format(command))
        os.chmod(target_path, 0o700)

    def configure_and_run_thininstaller_using_real_warehouse_policy(self, expected_return_code, policy_file_path, message_relays=None,
                                                                    proxy=None, update_caches=None, bad_url=False, args=None, mcs_ca=None, real=False):
        command = [self.default_installsh_path]
        if args:
            split_args = args.split(" ")
            for arg in split_args:
                command.append(arg)

        self.get_thininstaller()

        if not os.path.isfile(policy_file_path):
            raise OSError("%s file does not exist", policy_file_path)

        policy_file_name = os.path.basename(policy_file_path)
        warehouse_utils = WarehouseUtils.WarehouseUtils()
        try:
            template_config = warehouse_utils.get_template_config(policy_file_name)
            hashed_credentials = template_config.hashed_credentials
            connection_address = template_config.get_connection_address()
            warehouse_certs_dir = os.path.dirname(template_config.ps_root_ca)
        except KeyError:
            hashed_credentials, connection_address = extract_hashed_credentials_from_alc_policy(policy_file_path)
            warehouse_certs_dir = "system"


        logger.info("using creds: {}".format(hashed_credentials))
        logger.info("using certs: {}".format(warehouse_certs_dir))


        if bad_url:
            connection_address = None
        logger.info("connection address: {}".format(connection_address))

        self.create_default_credentials_file(update_creds=hashed_credentials, message_relays=message_relays, update_caches=update_caches)
        self.build_default_creds_thininstaller_from_sections()
        self.run_thininstaller(command, expected_return_code, None, mcs_ca=mcs_ca,
                               override_location=connection_address, certs_dir=warehouse_certs_dir, proxy=proxy, real=real)



    def build_default_creds_thininstaller_from_sections(self):
        self.build_thininstaller_from_sections(self.default_credentials_file_location, self.default_installsh_path)
        os.chmod(self.default_installsh_path, 0o700)

    def run_thininstaller_with_no_env_changes(self, expected_return_code=None):
        command = [self.default_installsh_path]
        logger.info("Running: " + str(command))
        log = open(self.log_path, 'w+')
        self.install_process = subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT)

        # Wait for execution to finish.
        self.install_process.communicate()
        rc = self.install_process.returncode
        logger.info("Thin Installer return code: {}".format(rc))
        log.flush()
        log.close()
        if expected_return_code is not None:
            if str(rc) != str(expected_return_code):
                self.dump_log()
                raise AssertionError("Thin Installer failed with exit code: " + str(rc) + " but was expecting: " + str(expected_return_code))

    def run_thininstaller(self, command, expected_return_code=0, mcsurl=None, mcs_ca=None, proxy=None, override_location="https://localhost:1233", override_path=None, certs_dir=None, real=False):
        cwd = os.getcwd()
        if not certs_dir:
            sophos_certs_dir = os.path.join(PathManager.get_support_file_path(), "sophos_certs")
            logger.info("sophos_certs_dir: {}".format(sophos_certs_dir))
        else:
            sophos_certs_dir = certs_dir
        if not mcs_ca:
            mcs_ca = os.path.join(PathManager.get_support_file_path(), "CloudAutomation/root-ca.crt.pem")
        if sophos_certs_dir == "system":
            logger.info("do not set override_sophos_certs")
            try:
                del self.env['OVERRIDE_SOPHOS_CERTS']
            except KeyError:
                pass
        else:
            self.env["OVERRIDE_SOPHOS_CERTS"] = sophos_certs_dir
        self.env["MCS_CA"] = mcs_ca
        if command[-1] not in ("--help", "-h", "--version") and not real:
            command.append("--allow-override-mcs-ca")
        if override_location:
            self.env["OVERRIDE_SOPHOS_LOCATION"] = override_location
        if mcsurl:
            self.env["OVERRIDE_CLOUD_URL"] = mcsurl
        self.env["DEBUG_THIN_INSTALLER"] = "1"
        if proxy:
            self.env["https_proxy"] = proxy
        if override_path is not None:
            self.env['PATH'] = override_path

        logger.info("env: {}".format(self.env))
        log = open(self.log_path, 'w+')
        logger.info("Running: " + str(command))
        self.install_process = subprocess.Popen(command, env=self.env, stdout=log, stderr=subprocess.STDOUT)

        # Wait for execution to finish.
        self.install_process.communicate()
        rc = self.install_process.returncode
        logger.info("Thin Installer return code: {}".format(rc))
        log.flush()
        log.close()
        if str(rc) != str(expected_return_code):
            self.dump_log()
            raise AssertionError("Thin Installer failed with exit code: " + str(rc) + " but was expecting: " + str(expected_return_code))

    def run_default_thininstaller(self,
                                  expected_return_code=0,
                                  mcsurl=None,
                                  override_location="https://localhost:1233",
                                  certs_dir=None,
                                  no_connection_address_override=False,
                                  proxy=None):
        if no_connection_address_override:
            override_location = None
        self.run_thininstaller([self.default_installsh_path],
                               expected_return_code,
                               mcsurl,
                               override_location=override_location,
                               certs_dir=certs_dir,
                               proxy=proxy)

    def run_real_thininstaller(self):
        cwd = os.getcwd()
        certs_dir = os.path.join(PathManager.get_support_file_path(), "sophos_certs", "prod_certs")
        mcs_ca = CentralUtils.get_nova_mcs_ca_path()
        self.run_thininstaller([self.default_installsh_path],
                               mcs_ca=mcs_ca,
                               override_location=None,
                               certs_dir=certs_dir,
                               real=True)

    def run_thininstaller_with_non_standard_path(self, expected_return_code, override_path, mcsurl=None):
        self.run_thininstaller([self.default_installsh_path],
                               expected_return_code,
                               override_path=override_path,
                               mcsurl=mcsurl)

    def create_fake_savscan_in_tmp(self):
        os.makedirs("/tmp/i/am/fake/bin/")
        open('/tmp/i/am/fake/bin/savscan', 'a').close()
        os.chmod("/tmp/i/am/fake/bin/savscan", stat.S_IXOTH)
        
    def remove_fake_savscan_in_tmp(self):
        shutil.rmtree("/tmp/i/")

    def create_fake_sweep_symlink(self, destination = "/usr/bin"):
        destination = os.path.join(destination, "sweep")
        link_target = "/tmp/i/am/fake/bin/savscan"
        os.symlink(link_target, destination)
        logger.info("made symlink at: {}".format(destination))
        
    def delete_fake_sweep_symlink(self, location = "/usr/bin"):
        location = os.path.join(location, "sweep")
        os.remove(location)
        logger.info("deleted symlink at: {}".format(location))

    def create_non_standard_sweep_symlink(self, dest):
        def safe_delete(p):
            try:
                os.unlink(p)
            except EnvironmentError:
                pass
        safe_delete("/usr/local/bin/sweep")
        safe_delete("/usr/local/bin/savscan")
        safe_delete("/usr/bin/sweep")
        safe_delete("/usr/bin/savscan")
        os.symlink("/opt/sophos-av/bin/savscan","/tmp/savscan")
        os.symlink("/opt/sophos-av/bin/savscan","/tmp/sweep")

    def run_default_thininstaller_with_env_proxy(self, expectedReturnCode, proxy):
        self.run_thininstaller([self.default_installsh_path], expectedReturnCode, proxy=proxy)

    def create_catalogue_directory(self):
        catalogue_path = os.path.join(self.installer_files, "bin", "warehouse", "catalogue")
        os.makedirs(catalogue_path)

    def run_default_thininstaller_with_fake_memory_amount(self, memory_in_kB):
        fake_mem_info_path = os.path.join(self.tmp_path, "fakememinfo")
        mem_info_path = "/proc/meminfo"
        with open(mem_info_path, "r") as mem_info:
            contents = mem_info.readlines()

        for index, line in enumerate(contents):
            if "MemTotal" in line:
                contents[index] = "MemTotal:       " + memory_in_kB + " kB"
                break

        with open(fake_mem_info_path, "w+") as fake_mem_info_file:
            fake_mem_info_file.writelines(contents)

        print ("Running installer with faked memory")
        bash_command_string = 'mount --bind ' + fake_mem_info_path + ' ' + mem_info_path + ' && bash -x ' + self.default_installsh_path
        command = ["unshare", "-m", "bash", "-x", "-c", bash_command_string]
        self.run_thininstaller(command, 4)

    def run_default_thininstaller_with_fake_small_disk(self):
        fake_small_disk_df_dir = os.path.join(PathManager.get_support_file_path(), "fake_system_scripts", "fake_df_small_disk")
        self.env["PATH"] = fake_small_disk_df_dir + ":" + os.environ['PATH']
        self.run_thininstaller([self.default_installsh_path], 5)

    def run_default_thininstaller_with_args(self, expectedReturnCode, *args):
        command = [self.default_installsh_path]
        for arg in args:
            command.append(arg)

        self.run_thininstaller(command, expectedReturnCode)

    def get_and_install_sav(self):
        tar_path = self.get_SAV_installer()
        self.install_SAV(tar_path)

    def find_latest_sav_tar(self):
        try:
            folder = BuiltIn().get_variable_value("${SAV_INPUT}")
        except robot.libraries.BuiltIn.RobotNotRunningError:
            folder = os.environ.get("SAV_INPUT", None)

        if not folder:
            raise AssertionError("Could not resolve SAV_INPUT environment variable")
        files = glob.glob(os.path.join(folder, 'sav-linux-x86-aggregated-10*.tgz'))
        if len(files) < 1:
            raise AssertionError("Could not find a SAV tar file in " + folder)
        return files[0]

    def get_SAV_installer(self):
        SAV_source_path = self.find_latest_sav_tar()
        shutil.copy(SAV_source_path, self.tmp_path)
        tar_name = os.path.basename(SAV_source_path)
        return os.path.join(self.tmp_path, tar_name)

    def install_SAV(self, sav_tar_file_name):
        # install the sav we have with arguments to make it non-interactive
        # Untar the SAV tar file

        os.system("tar -xvf ./{} -C {} > /dev/null".format(sav_tar_file_name, self.tmp_path))

        sav_install_sh = os.path.join(self.tmp_path, "sophos-av", "install.sh")
        sav_log_path = self.get_sav_log()
        cmd = "{} --acceptlicence --automatic --ignore-existing-directory --instdir /opt/sophos-av --enableOnAccess=False > {}"\
            .format(sav_install_sh, sav_log_path)

        if os.system(cmd) != 0:
           raise AssertionError("Could not install SAV")

    def uninstall_SAV(self):
        sav_dir = "/opt/sophos-av"
        sav_uninstaller = os.path.join(sav_dir, "uninstall.sh")
        if os.path.isfile(sav_uninstaller):
            if os.system(sav_uninstaller + " --force > /dev/null") != 0:
                raise AssertionError("Could not uninstall SAV")
        if os.path.isdir(sav_dir):
            shutil.rmtree(sav_dir)

    def get_sav_log(self):
        sav_log_path = os.path.join(self.tmp_path, "sav-install.log")
        return sav_log_path

    def get_main_installer_temp_location(self):
        list_of_files = glob.glob(os.path.join("/tmp", "SophosCentralInstall_*"))
        if len(list_of_files) == 0:
            raise AssertionError("Could not find the temp unpacked main installer dir")

        dir = max(list_of_files, key=os.path.getctime)

        print("Thin installer downloaded main installer to: {}".format(dir))
        return dir

    def remove_main_installer_temp_location(self):
        try:
            path = self.get_main_installer_temp_location()
        except AssertionError:
            print("no unpacked main installer found")
            return
        shutil.rmtree(path)

    def check_if_unwanted_strings_are_in_thininstaller(self, strings_to_check_for):
        # this is used for checking the content of the thininstaller header

        logger.info(self.default_installsh_path)

        with open(self.default_installsh_path, "rb") as installer:
            for line in installer:
                try:
                    line = line.decode("utf-8")
                    for string_value in strings_to_check_for:
                        if string_value.upper() in line.upper():
                            return True
                except UnicodeDecodeError:
                    # If we get here, it means we have reached the archive and
                    # none of the values being searched for have been found
                    pass
        return False

    def cleanup_systemd_files(self):
        subprocess.Popen(["rm", "-f", "/lib/systemd/system/sophos-spl.service"])
        subprocess.Popen(["rm", "-f", "/usr/lib/systemd/system/sophos-spl.service"])
        subprocess.Popen(["rm", "-f", "/lib/systemd/system/sophos-spl-update.service"])
        subprocess.Popen(["rm", "-f", "/usr/lib/systemd/system/sophos-spl-update.service"])
        subprocess.Popen(["systemctl", "daemon-reload"])

    def get_thininstaller_script(self):
        return os.path.join(self.installer_files, "SophosSetup.sh")

    def get_glibc_version_from_thin_installer(self):
        installer = self.get_thininstaller_script()
        build_variable_setting_line="BUILD_LIBC_VERSION="
        regex_pattern = r"{}([0-9]*\.[0-9]*)".format(build_variable_setting_line)
        with open(installer, "rb") as installer_file:
            for line in installer_file.readlines():
                line = line.decode("utf-8")
                if build_variable_setting_line in line:
                    match_object = re.match(regex_pattern, line)
                    version = match_object.group(1)
                    return version
        raise AssertionError("Installer: {}\nDid not contain line matching: {}".format(installer, regex_pattern))


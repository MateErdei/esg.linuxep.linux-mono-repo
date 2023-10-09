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
import FullInstallerUtils


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
                                                         "develop/sspl-thininstaller_lastgoodbuild.txt")
        self.https_certs_dir = os.path.join(PathManager.get_support_file_path(), "https/ca")

        try:
            os.makedirs(self.installer_files)
        except OSError as ex:
            if ex.errno != errno.EEXIST:
                raise

        self.default_credentials_file_location = os.path.join(self.installer_files, "credentials.txt")

    def get_default_install_script_path(self):
        return self.default_installsh_path

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

    def find_thininstaller_output(self, source=None):
        source = os.environ.get("THIN_INSTALLER_OVERRIDE", source)
        if source is not None:
            logger.info("using {} as source".format(source))
            return source

        local_dir = "/vagrant/esg.linuxep.thininstaller/output"
        if os.path.isdir(local_dir):
            logger.info("Thin Installer source: " + local_dir)
            return local_dir
        attempts = [local_dir]

        if os.path.isfile(self.last_good_artisan_build_file):
            attempts.append(self.last_good_artisan_build_file)

            with open(self.last_good_artisan_build_file) as f:
                last_good_build = f.read()

            attempts.append(last_good_build)

            print("Last good Thin Installer build: {}".format(last_good_build))
            source_folder = os.path.join("/mnt", "filer6", "bfr", "sspl-thininstaller", "develop", last_good_build, "sspl-thininstaller")
            version_dirs = os.listdir(source_folder)
            if len(version_dirs) == 1:
                source = os.path.join(source_folder, version_dirs[0], "output")
            else:
                raise AssertionError("More than one version in the build for thininstaller: {}".format(version_dirs))
            logger.info("using {} as source".format(source))
            return source

        raise AssertionError("Could not find thininstaller output in {}".format(attempts))

    def teardown_reset_original_path(self):
        self.env["PATH"] = self.original_path

    def cleanup_files(self):
        if os.path.isdir(self.tmp_path):
            shutil.rmtree(self.tmp_path)
        if os.path.lexists("/tmp/savscan"):
            os.unlink("/tmp/savscan")
        if os.path.lexists("/tmp/sweep"):
            os.unlink("/tmp/sweep")

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

    def create_credentials_file(self, token, url, update_creds, customer_token, message_relays, location, update_caches):
        new_line = '\n'
        with open(location, 'w') as fh:
            fh.write("TOKEN=" + token + new_line)
            fh.write("URL=" + url + new_line)
            fh.write("UPDATE_CREDENTIALS=" + update_creds + new_line)
            fh.write("CUSTOMER_TOKEN=" + customer_token + new_line)
            fh.write("PRODUCTS=all" + new_line)
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
                                        customer_token="ThisIsACustomerToken",
                                        message_relays=None,
                                        update_caches=None):
        self.create_credentials_file(token, url, update_creds, customer_token, message_relays, self.default_credentials_file_location, update_caches)

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


    def configure_and_run_SDDS3_thininstaller(self, expected_return_code,
                                                                    sus=None,
                                                                    cdn=None,
                                                                    use_http=False,
                                                                    message_relays=None,
                                                                    proxy=None,
                                                                    update_caches=None,
                                                                    args=None,
                                                                    mcs_ca=None,
                                                                    force_sdds3_post_install=False):
        command = ["bash", "-x", self.default_installsh_path]
        if args:
            split_args = args.split(" ")
            for arg in split_args:
                command.append(arg)

        self.get_thininstaller()

        self.create_default_credentials_file(message_relays=message_relays, update_caches=update_caches)
        self.build_default_creds_thininstaller_from_sections()
        self.run_thininstaller(command, expected_return_code, None, mcs_ca=mcs_ca, proxy=proxy, sus_url=sus, sdds3_cdn_url=cdn, sdds3_use_http=use_http, force_sdds3=force_sdds3_post_install)

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

    def run_thininstaller(self,
                          command,
                          expected_return_code=0,
                          mcsurl=None,
                          mcs_ca=None,
                          proxy=None,
                          override_location="https://localhost:1233",
                          override_path=None,
                          certs_dir=None,
                          force_certs_dir=None,
                          cleanup=True,
                          temp_dir_to_unpack_to=None,
                          sus_url="https://localhost:8080",
                          sdds3_cdn_url="https://localhost:8080",
                          sdds3_use_http=False,
                          force_sdds3=False,
                          force_legacy_install=False):
        cwd = os.getcwd()
        if not certs_dir:
            sophos_certs_dir = os.path.join(PathManager.get_support_file_path(), "sophos_certs")
            logger.info("sophos_certs_dir: {}".format(sophos_certs_dir))
        else:
            sophos_certs_dir = certs_dir
        if not mcs_ca:
            env_cert = os.environ.get("MCS_CA","")
            if os.path.isfile(env_cert):
                mcs_ca = env_cert
            else:
                mcs_ca = os.path.join(PathManager.get_support_file_path(), "CloudAutomation/root-ca.crt.pem")
        if sophos_certs_dir == "system":
            logger.info("do not set override_sophos_certs")
            try:
                del self.env['OVERRIDE_SOPHOS_CERTS']
            except KeyError:
                pass
        else:
            logger.info("Set sophos_certs_dir to: {}".format(sophos_certs_dir))
            test_using_prod = os.environ.get('TEST_USING_PROD', None)
            if test_using_prod:
                self.env["OVERRIDE_SOPHOS_CERTS"] = sophos_certs_dir
            elif force_certs_dir:
                self.env["OVERRIDE_SOPHOS_CERTS"] = force_certs_dir
            else:
                try:
                    del self.env['OVERRIDE_SOPHOS_CERTS']
                except KeyError:
                    pass
        self.env["MCS_CA"] = mcs_ca
        if command[-1] not in ("--help", "-h", "--version"):
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
        if not cleanup:
            self.env['OVERRIDE_INSTALLER_CLEANUP'] = "1"
        if temp_dir_to_unpack_to:
            self.env['SOPHOS_TEMP_DIRECTORY'] = temp_dir_to_unpack_to
        if sdds3_use_http:
            self.env['SDDS3_USE_HTTP'] = "1"
        if sdds3_cdn_url:
            self.env['OVERRIDE_CDN_LOCATION'] = sdds3_cdn_url
        if sus_url:
            self.env['OVERRIDE_SUS_LOCATION'] = sus_url
        if force_sdds3:
            self.env['USE_SDDS3'] = "1"
        if force_legacy_install:
            self.env['FORCE_LEGACY_INSTALL'] = "1"

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
                                  mcs_ca=None,
                                  override_location="https://localhost:1233",
                                  certs_dir=None,
                                  force_certs_dir=None,
                                  no_connection_address_override=False,
                                  proxy=None,
                                  installsh_path=None,
                                  cleanup=True,
                                  thininstaller_args=[]):
        if no_connection_address_override:
            override_location = None
        if not installsh_path:
            installsh_path = self.default_installsh_path
        self.run_thininstaller(["bash", "-x", installsh_path, *thininstaller_args],
                               expected_return_code,
                               mcsurl,
                               mcs_ca,
                               force_certs_dir=force_certs_dir,
                               override_location=override_location,
                               certs_dir=certs_dir,
                               proxy=proxy,
                               cleanup=cleanup)

    def run_default_thininstaller_with_different_name(self, new_filename, *args, **kwargs):
        new_filepath = os.path.join(os.path.dirname(self.default_installsh_path), new_filename)
        logger.info("new thin installer file path: {}".format(new_filepath))
        shutil.move(self.default_installsh_path, new_filepath)
        self.run_default_thininstaller(*args, installsh_path=new_filepath, **kwargs)

    def run_thininstaller_with_non_standard_path(self, expected_return_code, override_path, mcsurl=None, force_certs_dir=None):
        self.run_thininstaller([self.default_installsh_path],
                               expected_return_code,
                               override_path=override_path,
                               mcsurl=mcsurl,
                               force_certs_dir=force_certs_dir)

    def create_fake_savscan_in_tmp(self):
        os.makedirs("/tmp/i/am/fake/bin/")
        open('/tmp/i/am/fake/bin/savscan', 'a').close()
        os.chmod("/tmp/i/am/fake/bin/savscan", stat.S_IXOTH)

    def Create_Fake_SAV_Uninstaller_in_Tmp(self):
        fake_sav = "/tmp/i/am/fake"
        os.makedirs(fake_sav, exist_ok=True)
        uninstall = os.path.join(fake_sav, "uninstall.sh")
        f = open(uninstall, "w")
        f.write("""#!/bin/bash
touch /tmp/uninstalled
rm -rf %s
rm -f /usr/bin/sweep
exit 0""" % fake_sav)
        f.close()
        os.chmod(uninstall, stat.S_IRUSR | stat.S_IXUSR)

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

    def run_default_thinistaller_with_product_args_and_central(self, expectedReturnCode, force_certs_dir, product_argument="", mcsurl=None):
        command = [self.default_installsh_path]
        if product_argument != "":
            command.append(product_argument)

        self.run_thininstaller(command, expectedReturnCode, mcsurl=mcsurl, force_certs_dir=force_certs_dir)

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

    def replace_register_central_with_script_that_echos_args(self):
        register_central = os.path.join("/opt","sophos-spl","base","bin","registerCentral")
        output_file_path = os.path.join("/tmp","registerCentralArgs")
        os.remove(register_central)
        with open(register_central, "w") as file:
            file.write(f'#!/bin/bash\necho "$@">{output_file_path}')
        os.chmod(register_central, 0o777)
        return output_file_path
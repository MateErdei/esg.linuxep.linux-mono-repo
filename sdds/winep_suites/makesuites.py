import os
import shutil
import subprocess
from xml.etree import ElementTree as ET


def copy_build_inputs_to_appropriate_suites():
    copies = [
        [r'savcfg\UI_home_SYNC\SAVCFG',       r'suite_data\WindowsCloudHome\data\savxp\SAVCFG'],
        [r'savcfg\UI_no_install_SYNC\SAVCFG', r'suite_data\WindowsCloudAV\data\savxp\SAVCFG'],
        [r'savcfg\UI_install\SAVCFG',         r'suite_data\WindowsCloudServerLegacy\data\savxp\SAVCFG'],
        [r'savcfg\UI_no_install_SYNC\SAVCFG', r'suite_data\WindowsCloudServerAV\data\savxp\SAVCFG'],
        [r'savcfg\OPM_install_epa_hu\SAVCFG', r'suite_data\StandaloneWindows\data\savxp\SAVCFG'],
        [r'savcfg\OPM_install_epa_hu\SAVCFG', r'suite_data\OnPremAdvanced\data\savxp\SAVCFG'],
        [r'savcfg\OPM_install_epa_hu\SAVCFG', r'suite_data\OnPremAdvancedLight\data\savxp\SAVCFG'],
        [r'savcfg\OPM_install_epa_hu\SAVCFG', r'suite_data\OnPremStandard\data\savxp\SAVCFG'],
        [r'savcfg\OPM_install_epa_hu\SAVCFG', r'suite_data\OnPremStandardLight\data\savxp\SAVCFG'],
        [r'savcfg\OPM_install_epa_hu\SAVCFG', r'suite_data\ES2SCPXP_safpmt\data\savxp\SAVCFG'],
        [r'savcfg\OPM_install_epa_hu\SAVCFG', r'suite_data\ESEPBXP\data\savxp\SAVCFG'],
    ]
    for f, t in copies:
        print(f'Copying {f} -> {t}')
        subprocess.check_call(['xcopy', '/YI', f, t])


SED_CONFIG_TREATMENT = [
    'WindowsCloudHome',
    'WindowsCloudNextGen',
    'WindowsCloudNextGenEap',
    'WindowsCloudServer',
]


def cleanup_sed_config(suite_dir, subdir):
    data_directory = os.path.join(suite_dir, subdir)

    for junk in ('sed64', 'sedarm64', '@ssp'):
        jdir = os.path.join(data_directory, 'sed64')
        if os.path.exists(jdir):
            shutil.rmtree(jdir)


def clone_suite(suite_dir, subdir, tgtdir):
    data_directory = os.path.join(suite_dir, subdir)
    if not os.path.exists(data_directory):
        raise FileNotFoundError(f'Error: data directory {data_directory} not found')
    clone_directory = os.path.join(suite_dir, tgtdir)
    if os.path.exists(clone_directory):
        shutil.rmtree(clone_directory)
    shutil.copytree(data_directory, clone_directory)


def move_sed_config(data_dir, confdir, copydir):
    confdir = os.path.join(data_dir, confdir)
    copydir = os.path.join(data_dir, copydir)
    if not os.path.exists(confdir):
        raise FileNotFoundError(f'Error: sed config directory {confdir} not found')
    if os.path.exists(copydir):
        raise FileNotFoundError(f'Error: sed config directory {copydir} already exists')
    os.rename(confdir, copydir)


def copy_sed_config(data_dir, confdir, copydir):
    confdir = os.path.join(data_dir, confdir)
    copydir = os.path.join(data_dir, copydir)
    if not os.path.exists(confdir):
        raise FileNotFoundError(f'Error: sed config directory {confdir} not found')
    if os.path.exists(copydir):
        raise FileNotFoundError(f'Error: sed config directory {copydir} already exists')
    shutil.copytree(confdir, copydir)


def make_sdds_package(suite_dir, data):
    data_directory = os.path.join(suite_dir, data)

    if not os.path.exists(data_directory):
        os.makedirs(data_directory)

    sddsimportfile = os.path.join(data_directory, "SDDS-Import.xml")
    if os.path.exists(sddsimportfile):
        print("Deleting", sddsimportfile)
        os.unlink(sddsimportfile)

    subprocess.check_call([
        "py", "-3", "-u", "make_sdds_import.py",
        "-d", os.path.join(suite_dir, "spv_template.xml"),
        "-f", data_directory
    ])

    sdffile = os.path.join(data_directory, "sdf.xml")
    if os.path.exists(sdffile):
        print("Checking:", sdffile)
        numpackages = count_subpackages(sdffile)
        if os.path.exists(os.path.join(data_directory, "PRODUCT_RETIRED.txt")):
            if numpackages != 0:
                print(f"ERROR: Problem processing {suite_dir} Retired packages should never have any subcomponents")
                success = False
        else:
            # its a normal package
            if numpackages == 0:
                print(f"ERROR: Problem processing {suite_dir} normal packages should always have subcomponents")
                success = False


def count_subpackages(sdffile):
    t = ET.parse(sdffile)
    subpackages = t.find(".//SubPackages")
    return len(subpackages)


def main():
    copy_build_inputs_to_appropriate_suites()

    suite_dirs = os.listdir("suite_data")
    print("detected the following suites", suite_dirs)

    success = True
    for d in suite_dirs:
        suite_dir = os.path.join("suite_data", d)
        print("importing", suite_dir)

        if d in SED_CONFIG_TREATMENT:
            cleanup_sed_config(suite_dir, 'data')

            clone_suite(suite_dir, 'data', 'sdds3')
            move_sed_config(f'{suite_dir}/sdds3', 'sed', '@ssp')
            make_sdds_package(suite_dir, 'sdds3')

            copy_sed_config(f'{suite_dir}/data', 'sed', 'sed64')

        make_sdds_package(suite_dir, 'data')


    assert success, 'Failed to make suites'


if __name__ == '__main__':
    main()

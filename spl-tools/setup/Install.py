#!/usr/bin/env python3
import sys
import os
import subprocess as sp
import argparse

TMPSCRIPT = "tmpscript.sh"
REMOTEDIR = "/vagrant"
INSTALLEXAMPLEPLUGIN = False


def parse_arguments():
    parser = argparse.ArgumentParser(description="Install Base and Plugins to an Ubuntu Vagrant VM")
    parser.add_argument('--mcsurl', dest="mcsurl", action="store",
                        help="MCS URL for Central (No Central connection if unset)")
    parser.add_argument('--mcstoken', dest="mcstoken", action="store",
                        help="MCS Token for Central (No Central connection if unset)")
    parser.add_argument("--exampleplugin", dest="exampleplugin", default=False, action="store_true",
                        help="Install exampleplugin")
    parser.add_argument("--noVagrant", dest="noVagrant", default=False, action="store_true",
                        help="Install SSPL to the Host machine, rather than a vagrant VM")
    return parser.parse_args()


# auxiliary method to find the VagrantRoot folder (where the Vagrantfile is)
def find_vagrant_root(start_from_dir):
    curr = start_from_dir
    while not os.path.exists(os.path.join(curr, 'Vagrantfile')):
        curr = os.path.split(curr)[0]
        if curr in ['/', '/home']:
            raise AssertionError('Failed to find Vagrantfile in directory or its parents: {}'.format(start_from_dir))
    return curr


def check_vagrant_up_and_running():
    output = sp.check_output(['/usr/bin/vagrant', 'status'])
    if b'running' not in output:
        print ('starting up vagrant')
        sp.call(['/usr/bin/vagrant', 'up', 'ubuntu'])


def is_installer_path_valid(installer_path):
    if "everest-base" in installer_path:
        return False
    if "sspl-plugin-template" in installer_path:
        return False
    if not INSTALLEXAMPLEPLUGIN and "exampleplugin" in installer_path:
        return False
    if "build64/sdds" not in installer_path:
        return False
    return True


def find_all_installers():
    result = []
    for root, dir, file in os.walk("."):
        if "install.sh" in file and is_installer_path_valid(root):
            result.append(os.path.join(root, "install.sh"))
    return result

def run_tempfile(contents):
    with open(TMPSCRIPT, 'w') as f:
        f.write(contents)
    sp.call(["bash", f"{TMPSCRIPT}"])

def run_tempfile_on_vagrant(contents):
    with open(TMPSCRIPT, 'w') as f:
        f.write(contents)
    print("Connecting to 127.0.0.1 (vagrant)")
    vagrant_cmd = ['/usr/bin/vagrant', 'ssh', 'ubuntu', '-c', f"bash {REMOTEDIR}/{TMPSCRIPT}"]
    sp.call(vagrant_cmd)

def install_base(url, token, noVagrant):
    print("Installing Base")
    print(f"MCSURL={url} MCSTOKEN={token}")
    script = f"""
    
# Uninstall Base if already installed
[[ -d /opt/sophos-spl ]] && sudo /opt/sophos-spl/bin/uninstall.sh --force

# Install Base, setting MCS URL/Token if necessary
sudo {REMOTEDIR if not noVagrant else "."}/everest-base/output/SDDS-COMPONENT/install.sh \
{"--mcs-token " + token if token else ""} \
{"--mcs-url " + url if url else ""}

"""
    if noVagrant:
        run_tempfile(script)
    else:
        run_tempfile_on_vagrant(script)


def install_plugins(noVagrant):
    for installer in find_all_installers():
        if noVagrant:
            script = f"sudo {installer}"
        else:
            script = f"sudo {os.path.join(REMOTEDIR, installer)}"
        print(f"Running {installer}")
        if noVagrant:
            run_tempfile(script)
        else:
            run_tempfile_on_vagrant(script)


def main():
    global INSTALLEXAMPLEPLUGIN

    args = parse_arguments()
    INSTALLEXAMPLEPLUGIN = args.exampleplugin

    currdir = os.path.abspath(os.getcwd())
    vagrantroot = find_vagrant_root(currdir)
    os.chdir(vagrantroot)

    if not args.noVagrant:
        check_vagrant_up_and_running()

    install_base(args.mcsurl, args.mcstoken, args.noVagrant)
    install_plugins(args.noVagrant)

    if os.path.exists(TMPSCRIPT):
        os.remove(TMPSCRIPT)
    os.chdir(currdir)


if __name__ == "__main__":
    sys.exit(main())

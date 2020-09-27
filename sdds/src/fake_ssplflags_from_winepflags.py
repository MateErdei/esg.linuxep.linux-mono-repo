import os
import shutil
import sys


def rearrange_files(flags_input_path):  # flags_input_path = ..\redist\flags
    if not os.path.exists(flags_input_path):
        return 1

    sspl_flags_root = os.path.join(flags_input_path, r"sspl_flags")

    shutil.move(os.path.join(flags_input_path, r"union\LATEST\mcsep"), sspl_flags_root)
    shutil.copy(os.path.join(flags_input_path, r"union\LATEST\SDDS-Import.xml"), sspl_flags_root)
    sspl_flags_dist = os.path.join(flags_input_path, r"sspl_flags\files\base\etc\sophosspl")
    os.makedirs(os.path.dirname(sspl_flags_dist))
    shutil.move(os.path.join(sspl_flags_root, "flags"), sspl_flags_dist)
    #sb_manifest sign  sspl_flags_root

    return 0


if __name__ == "__main__":
    sys.exit(rearrange_files(r"..\redist\flags"))

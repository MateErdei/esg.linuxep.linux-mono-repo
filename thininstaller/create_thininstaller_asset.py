import json
import os
import shutil
import subprocess

import FileInfo
import sys


def main(argv):
    installer_tar = argv[1]
    compatibility_checker_path = argv[2]
    zipper_tool = argv[3]
    output_zip_path = argv[4]

    file_info = FileInfo.FileInfo(".", installer_tar)
    sha256 = file_info.m_sha256

    shutil.copy(installer_tar, f"{sha256}.tar.gz")

    with open("latest.json", "w") as f:
        latest_json = {"name": sha256}
        f.write(json.dumps(latest_json))

    zipper_tool_parameters = f"{output_zip_path}_params"
    with open(zipper_tool_parameters, "w") as f:
        f.writelines([
            "--zip\n",
            f"{output_zip_path}\n",
            "--sha256\n",
            f"{output_zip_path}.sha256\n",
            "--manifest\n",
            f"{output_zip_path}.manifest.json\n",
            "--add\n",
            "latest.json=latest.json\n",
            "--add\n",
            f"{compatibility_checker_path}=SPLCompatibilityChecks.sh\n"
            "--add\n",
            f"{sha256}.tar.gz={sha256}.tar.gz\n"
        ])

    subprocess.run([zipper_tool, f"@{zipper_tool_parameters}"])

    os.remove(f"{sha256}.tar.gz")
    os.remove("latest.json")
    os.remove(zipper_tool_parameters)


if __name__ == '__main__':
    sys.exit(main(sys.argv))

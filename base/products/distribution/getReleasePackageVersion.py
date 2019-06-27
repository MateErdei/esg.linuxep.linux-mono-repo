import xml.dom.minidom
import os
import sys

def main(argv):
    path_to_release_package = argv[1]
    if not os.path.exists(path_to_release_package):
        sys.stderr.write("ERROR - path: {} \tdoes not exist\n".format(path_to_release_package))
        return 1
    try:
        with open(path_to_release_package) as release_package:
            document = xml.dom.minidom.parse(release_package)
            top_level_node = document.childNodes[0]
            version = top_level_node.getAttribute("version")
    except Exception as e:
        sys.stderr.write("failed to get version from release-package.xml with error: {}\n".format(e))
        return 2

    sys.stdout.write(version)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))

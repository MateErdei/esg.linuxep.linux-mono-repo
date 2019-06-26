import xml.dom.minidom
import os
import sys

def main(argv):
    os.chdir(os.path.dirname(os.path.realpath(__file__)))

    path_to_release_package = os.path.join("..", "..", "build-files", "release-package.xml")
    try:
        with open(path_to_release_package) as release_package:
            document = xml.dom.minidom.parse(release_package)
            top_level_node = document.childNodes[0]
            version = top_level_node.getAttribute("version")
    except Exception as e:
        print("failed to get version from release-package.xml with error: {}".format(e))
        version = None

    return version


if __name__ == '__main__':
    sys.exit(sys.stdout.write(main(sys.argv)))

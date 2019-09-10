import os
import sys
from xml.etree import ElementTree as ET
from shutil import copytree, rmtree



def check_if_versionable_componet_suite(sdds):
    for rigid_name in sdds.findall(".//Component//RigidName"):
        StringforRigidName = rigid_name.text
    return StringforRigidName

def change_sddsimport(sdds, sdds_componentsuite, version_number):
    for versioning in sdds.findall(".//Component//Version"):
        versioning.text = version_number
    for versioning in sdds.findall(".//Component//Build"):
        versioning.text = version_number
    for versioning in sdds.findall(".//Component//EMLVersion"):
        versioning.text = version_number
    for versioning in sdds.findall(".//Dictionary//Name//Label//Token"):
        versionablecomponetsuite = check_if_versionable_componet_suite(sdds)
        versioning.text = versionablecomponetsuite + "#" + version_number
    for versioning in sdds.findall(".//Dictionary//Name//Label//Language//ShortDesc"):
        versioning.text = version_number
    for versioning in sdds.findall(".//Dictionary//Name//Label//Language//LongDesc"):
        versioning.text = "Sophos Enterprise Console v" + version_number 
    sdds.write(sdds_componentsuite)

if __name__ == '__main__':
    sdds_componentsuite_location = sys.argv[1]
    sdds_componentsuite = os.path.join(sdds_componentsuite_location, "sdds-import.xml")
    sdds = ET.parse(sdds_componentsuite)
    version_number = sys.argv[2]
    output_location = sys.argv[3]+ version_number
    change_sddsimport(sdds, sdds_componentsuite, version_number)
    copytree(sdds_componentsuite_location, output_location)

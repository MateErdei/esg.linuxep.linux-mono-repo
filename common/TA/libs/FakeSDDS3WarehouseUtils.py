import os
import hashlib
BASE_SUITE_TEMPLATE="""
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<suite name="ServerProtectionLinux-Base" version="2022.7.22.7" nonce="3b1805f739" marketing-version="2022.7.22.7 (develop)">
  <package-ref src="@FILENAME@" size="@SIZE@" sha256="@SHA@">
    <name>SPL-Base-Component</name>
    <version>@VERSION@</version>
    <line-id>ServerProtectionLinux-Base-component</line-id>
    <nonce>@NONCE@</nonce>
    <description>Sophos Linux Protection Base Component v1.0.0</description>
    <decode-path>ServerProtectionLinux-Base-component</decode-path>
    <features>
      <feature name="CORE" />
    </features>
    <platforms>
      <platform name="LINUX_INTEL_LIBC6" />
      <platform name="LINUX_ARM64" />
    </platforms>
    <supplement-ref src="SSPLFLAGS" tag="2022-1" decode-path="sspl_flags/files/base/etc/sophosspl" />
  </package-ref>
</suite>
"""
SUPPLEMENT_XML="""
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<supplement name="SSPLFLAGS" timestamp="2022-07-27T13:02:27Z">
  <package-ref src="@FILENAME@" size="@SIZE@" sha256="@SHA@">
    <name>SSPLFLAGS</name>
    <version>@VERSION@</version>
    <line-id>SSPLFLAGS</line-id>
    <nonce>020fb0c370</nonce>
    <description>SSPLFLAGS Supplement Data v2022.7.27.6.1</description>
    <tags>
      <tag name="2022-1" />
    </tags>
  </package-ref>
</supplement>
"""
FLAG_XML_TEMPLATE="""
{
  "RECOMMENDED": {
    "suite": "sdds3.ServerProtectionLinux-Base_2022.7.22.7.020fb0c370.dat",
    "version": "2022.7.22.7"
  }
}
"""
default_package_path="/tmp/sdds3FakeWarehouse/fakerepo/package/SPL-Base-Component_1.0.0.0.020fb0c370.zip"
default_output_path="/tmp/sdds3FakeWarehouse/fakerepo"
default_flag_path="/tmp/sdds3FakeWarehouse/fakeflag"

def write_sdds3_suite_xml(package_path=default_package_path,output_path=default_output_path):
    size = os.path.getsize(package_path)
    sha = 0
    with open(package_path, "rb") as f:
        content = f.read()
        sha = hashlib.sha256(content).hexdigest()
    filename = os.path.basename(package_path)
    version = filename[19:-15]
    nonce = filename.split(".")[4]

    print(version)
    print(nonce)
    suite_xml = BASE_SUITE_TEMPLATE.replace("@SIZE@", str(size)).replace("@SHA@", sha).replace("@FILENAME@", filename).replace("@VERSION@",version).replace("@NONCE@", nonce)
    with open(os.path.join(output_path ,"suite.xml"), "w") as f:
        f.write(suite_xml)

def write_sdds3_supplement_xml(package_path,output_path=default_output_path):
    size = os.path.getsize(package_path)
    sha = 0
    with open(package_path, "rb") as f:
        content = f.read()
        sha = hashlib.sha256(content).hexdigest()
    filename = os.path.basename(package_path)
    version = filename[10:-15]
    print(version)
    suite_xml = SUPPLEMENT_XML.replace("@SIZE@", str(size)).replace("@SHA@", sha).replace("@FILENAME@", filename).replace("@VERSION@",version)
    with open(os.path.join(output_path ,"supplement.xml"), "w") as f:
        f.write(suite_xml)


def write_sdds3_flag(output_path=default_flag_path):
    with open(os.path.join(output_path ,"release.linuxep.ServerProtectionLinux-Base.json"), "w") as f:
        f.write(FLAG_XML_TEMPLATE)
import subprocess
import sys
import os


def check_call(cmd):
    print "calling (check)", cmd
    subprocess.check_call(cmd)


def call(cmd):
    print "calling", cmd
    subprocess.call(cmd)

print "Build starting..."

print "Make sure any components already fetched are dev signed"
check_call(["python", "update_all_manifests_and_imports.py", ".."])
print "success"

print "Combining warehouse defintion with common component and supplement definitions..."
check_call(["python", "definition-merger.py", "-o", r"..\output\def\def.yaml"] + sys.argv[5:])

# From the ostia root (e.g. http://ostia.eng.sophos/dev) to the actual warehouse
rel_ostia_link = "/".join([sys.argv[1], sys.argv[2].replace("/", "-")])

print "Fetch components and generate warehouse definition..."
check_call(["python", "warehouse-def.py", "-v", "-c",
            rel_ostia_link, "-b", sys.argv[3], "-o", r"..\output\def\def.xml",
            "--bom", r"..\logs\bom.json", r"..\output\def\def.yaml"])

print "Components fetched and warehouse definition generated successfully!"


# **********************
# NOTE: SDDSImport.exe sometimes crashes even though it has completed its
# operations successfully.
# So we can't rely on its exit code, we need to look in its log output to
# check if all operations have succeeded
# **********************

print "Importing dictionary file..."
call(["cmd.exe", "/c",
                 r"c:\program files\Sophos Plc\SDDS\SDDSImport.exe", "-server",
                 os.environ["COMPUTERNAME"], "-user", "admin", "-verbose",
                 "-file", sys.argv[4], ">",
                 r"..\logs\b_dict_import.log", "2>&1"])
call(["cmd.exe", "/c", "type", r"..\logs\b_dict_import.log"])
check_call(
    ["cscript.exe", "//nologo", "SDDSImport-ErrorCheck.vbs", "-logfile",
     r"..\logs\b_dict_import.log"])

print "Dictionary file imported successfully!"


print "Importing warehouse..."
call(["cmd.exe", "/c",
                 r"c:\program files\Sophos Plc\SDDS\SDDSImport.exe", "-server",
                 os.environ["COMPUTERNAME"],
                 "-user", "admin", "-verbose", "-file",
                 r"..\output\def\def.xml", ">", r"..\logs\c_warehouse_import.log",
                 "2>&1"])
call(["cmd.exe", "/c", "type", r"..\logs\c_warehouse_import.log"])
check_call(
    ["cscript.exe", "//nologo", "SDDSImport-ErrorCheck.vbs",
     "-logfile", r"..\logs\c_warehouse_import.log"])

print "Warehouse imported successfully!"


print "Generating customer file..."
check_call(["python", "CustomerFile.py", "-v", "-c", rel_ostia_link,
            "-b", sys.argv[3], r"..\output\def\def.yaml"])

print "Customer file generated successfully!"

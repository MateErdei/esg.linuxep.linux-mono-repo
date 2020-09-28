import subprocess
import sys
import os

import definition_merger


def check_call(cmd):
    print("calling (check)", cmd)
    subprocess.check_call(cmd)


def call(cmd):
    print("calling", cmd)
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

print("Build starting...", sys.argv)

# **********************
# NOTE: SDDSImport.exe sometimes crashes even though it has completed its
# operations successfully.
# So we can't rely on its exit code, we need to look in its log output to
# check if all operations have succeeded
# **********************

SDDS_IMPORT_PATH = r"c:\program files\Sophos Plc\SDDS\SDDSImport.exe"
SDDS_SERVER = os.environ['COMPUTERNAME']


def sdds_import(path, name):
    os.makedirs('logs', exist_ok=True)
    args = [SDDS_IMPORT_PATH, '-server', SDDS_SERVER, '-user', 'Admin', '-verbose', '-file', path]
    log_path = os.path.join('logs', f'sddsimport_{name}.log')
    try:
        with open(log_path, 'wb') as log:
            subprocess.check_call(args, stdout=log, stderr=log)
    except subprocess.CalledProcessError as e:
        print(f'WARNING: SddsImport exited with {e.returncode}. Continuing.')
        print(f'WARNING: SddsImport log_path: {log_path}')
    finally:
        with open(log_path, 'rb') as log:
            print(log.read().decode('latin-1'))
    sys.stdout.flush()

    check_call(["cscript.exe", "//nologo", "SDDSImport-ErrorCheck.vbs", "-logfile", log_path])


print("Importing dictionary file...")
sdds_import(path=sys.argv[1], name='dictionary')
print("Dictionary file imported successfully!")


print("Combining warehouse defintion with common component and supplement definitions...")
definition_merger.merge(r'..\output\def\def.yaml', input_files=sys.argv[2:])


print("Fetch components and generate warehouse definition...")
check_call(["python", "warehouse-def.py", "-v", "-o", r"..\output\def\def.xml",
            "--bom", r"..\logs\bom.json", r"..\output\def\def.yaml"])
print("Components fetched and warehouse definition generated successfully!")

print("Importing warehouse...")
sdds_import(path=r'..\output\def\def.xml', name='warehouse')
print("Warehouse imported successfully!")

print("Generating customer file...")
check_call(["python", "CustomerFile.py", "-v", r"..\output\def\def.yaml"])
print("Customer file generated successfully!")

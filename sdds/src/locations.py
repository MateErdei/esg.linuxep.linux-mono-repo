import os
import os.path

cwd = os.getcwd()
LOGS = os.path.join(cwd, "..", "logs")
WAREHOUSE_WRITE = os.path.join(cwd, "..", "output", "warehouse", "content")
CUSTOMER_FILES_WRITE = os.path.join(cwd, "..", "output", "warehouse")
branch = os.environ.get("PIPELINE_NAME")
WAREHOUSE_READ = [
    "http://ostia.eng.sophos/dev/sspl-warehouse/{}/warehouse/warehouse".format(branch),
    "http://d1.sophosupd.net/update/"
]
REPOSITORY_ID = "ES+C"

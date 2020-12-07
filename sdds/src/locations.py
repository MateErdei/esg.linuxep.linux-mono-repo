import os
import os.path

cwd = os.getcwd()
LOGS = os.path.join(cwd, "..", "logs")
WAREHOUSE_WRITE = os.path.join(cwd, "..", "output", "warehouse", "content")
CUSTOMER_FILES_WRITE = os.path.join(cwd, "..", "output", "warehouse")
WAREHOUSE_READ = ["http://d1.sophosupd.net/update/"]
REPOSITORY_ID = "ES+C"

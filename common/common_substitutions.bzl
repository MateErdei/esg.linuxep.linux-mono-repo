# Copyright 2023 Sophos Limited. All rights reserved.
USER_AND_GROUP_SUBSTITUTIONS = {
    "@SOPHOS_SPL_USER@": "sophos-spl-user",
    "@SOPHOS_SPL_GROUP@": "sophos-spl-group",
    "@SOPHOS_SPL_IPC_GROUP@": "sophos-spl-ipc",
    "@SOPHOS_SPL_LOCAL@": "sophos-spl-local",  # low privilege user
    "@SOPHOS_SPL_UPDATESCHEDULER@": "sophos-spl-updatescheduler",  # user which updatescheduler runs as
}

PYTHON_SUBSTITUTIONS = {
    "@PYTHONPATH@": "$pythonzip:$mcsrouterzip",
    "@PYTHON_ARGS_FOR_PROD@": "",
    "@PRODUCT_PYTHON_EXECUTABLE@": "$BASE_DIR/bin/python3",
    "@PRODUCT_PYTHONHOME@": "$INST_DIR/base/",
    "@PRODUCT_PYTHON_LD_LIBRARY_PATH@": "$INST_DIR/base/lib:$INST_DIR/base/lib64",
    "@PYTHON_ZIP@": "python311.zip",
}

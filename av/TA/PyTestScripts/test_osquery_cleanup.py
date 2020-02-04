import os


def detect_failure(func):
    def wrapper_function(sspl_mock, edr_plugin_instance):
        try:
            v = func(sspl_mock, edr_plugin_instance)
            return v
        except:
            edr_plugin_instance.set_failed()
            raise
    return wrapper_function

# In order to verify the cleanup algorithms which always run on startup and periodically (every 1hour) all these tests use
# the start up to enter the clean up


@detect_failure
def test_edr_plugin_purges_database_when_threshold_reached(sspl_mock, edr_plugin_instance):
    edr_plugin_instance.prepare_for_test()
    i = 105

    # Make sure the directory for the db files is there so we can put the test files in place
    os.makedirs(edr_plugin_instance.osquery_database_path(), exist_ok=True)
    while i > 0:
        db_path = edr_plugin_instance.osquery_database_path()
        path = os.path.join(db_path, str(i))
        with open(path, 'a')as file:
            file.write("blah")
        i -= 1

    edr_plugin_instance.start_edr()

    assert edr_plugin_instance.wait_log_contains("Purging Database")
    assert edr_plugin_instance.wait_log_contains("Purging Done", 15)
    files = os.listdir(db_path)
    assert len(files) < 10

@detect_failure
def test_edr_plugin_rotates_logfiles_when_threshold_reached(sspl_mock, edr_plugin_instance):
    edr_plugin_instance.prepare_for_test()
    i = 10
    while i > 0:
        log_path = edr_plugin_instance.log_path()
        path = os.path.join(log_path, "osqueryd.results.log." + str(i))
        with open(path, 'a')as file:
            file.write("blah" + str(i))
        i -= 1

    file_to_rotate = os.path.join(log_path, "osqueryd.results.log")
    i = 1024*1024 + 10

    with open(file_to_rotate, 'ab')as file:
        file.write(b"\0"*i)

    edr_plugin_instance.start_edr()

    assert edr_plugin_instance.wait_log_contains("Rotating osquery logs")
    assert edr_plugin_instance.wait_log_contains("Log limit reached : Deleting oldest osquery log file")

    i = 10

    while i > 1:
        log_path = edr_plugin_instance.log_path()
        path = os.path.join(log_path, "osqueryd.results.log." + str(i))
        with open(path, 'r')as file:
            line = file.read()
        assert line == "blah" + str(i-1)
        i -= 1

    actual_files = os.listdir(log_path).sort()
    expected_files = ["edr.log",
                      "osqueryd.results.log",
                      "osqueryd.results.log.1",
                      "osqueryd.results.log.10",
                      "osqueryd.results.log.2",
                      "osqueryd.results.log.3",
                      "osqueryd.results.log.4",
                      "osqueryd.results.log.5",
                      "osqueryd.results.log.6",
                      "osqueryd.results.log.7",
                      "osqueryd.results.log.8",
                      "osqueryd.results.log.9"].sort()

    assert actual_files == expected_files


@detect_failure
def test_edr_plugin_removes_old_warning_files_when_threshold_reached(sspl_mock, edr_plugin_instance):
    edr_plugin_instance.prepare_for_test()
    warning_paths = []
    log_path = edr_plugin_instance.log_path()
    warning_paths.append(os.path.join(log_path, "osqueryd.WARNING.20200117-042121.1001"))
    warning_paths.append(os.path.join(log_path, "osqueryd.WARNING.20200117-042121.1002"))
    warning_paths.append(os.path.join(log_path, "osqueryd.WARNING.20200117-042121.1003"))
    warning_paths.append(os.path.join(log_path, "osqueryd.WARNING.20200117-042121.1004"))
    warning_paths.append(os.path.join(log_path, "osqueryd.WARNING.20200117-042121.1005"))
    warning_paths.append(os.path.join(log_path, "osqueryd.WARNING.20200117-042121.1006"))
    warning_paths.append(os.path.join(log_path, "osqueryd.WARNING.20200117-042121.1007"))
    warning_paths.append(os.path.join(log_path, "osqueryd.WARNING.20200117-042121.1008"))
    warning_paths.append(os.path.join(log_path, "osqueryd.WARNING.20200117-042121.1009"))
    warning_paths.append(os.path.join(log_path, "osqueryd.WARNING.20200117-042121.1010"))
    warning_paths.append(os.path.join(log_path, "osqueryd.WARNING.20200117-042121.1011"))
    warning_paths.append(os.path.join(log_path, "osqueryd.WARNING.20200117-042121.1012"))

    for path in warning_paths:
        with open(path, 'a')as file:
            file.write("blah")

    edr_plugin_instance.start_edr()

    assert edr_plugin_instance.wait_log_contains("Removed old osquery WARNING file:")

    preserved_files = []
    end_paths = os.listdir(log_path)
    for path in warning_paths:
        if os.path.basename(path) in end_paths:
            print("preserved file: {}".format(path))
            preserved_files.append(path)

    assert len(preserved_files) == 10
    assert os.path.join(log_path, "osqueryd.WARNING.20200117-042121.1012") in preserved_files
    removed_files = list(set(warning_paths) - (set(preserved_files)))
    assert len(removed_files) == 2
    assert os.path.join(log_path, "osqueryd.WARNING.20200117-042121.1001") in removed_files


@detect_failure
def test_edr_plugin_removes_old_info_files_when_threshold_reached(sspl_mock, edr_plugin_instance):
    edr_plugin_instance.prepare_for_test()

    info_paths = []
    log_path = edr_plugin_instance.log_path()
    info_paths.append(os.path.join(log_path, "osqueryd.INFO.20200117-042121.1001"))
    info_paths.append(os.path.join(log_path, "osqueryd.INFO.20200117-042121.1002"))
    info_paths.append(os.path.join(log_path, "osqueryd.INFO.20200117-042121.1003"))
    info_paths.append(os.path.join(log_path, "osqueryd.INFO.20200117-042121.1004"))
    info_paths.append(os.path.join(log_path, "osqueryd.INFO.20200117-042121.1005"))
    info_paths.append(os.path.join(log_path, "osqueryd.INFO.20200117-042121.1006"))
    info_paths.append(os.path.join(log_path, "osqueryd.INFO.20200117-042121.1007"))
    info_paths.append(os.path.join(log_path, "osqueryd.INFO.20200117-042121.1008"))
    info_paths.append(os.path.join(log_path, "osqueryd.INFO.20200117-042121.1009"))
    info_paths.append(os.path.join(log_path, "osqueryd.INFO.20200117-042121.1010"))
    info_paths.append(os.path.join(log_path, "osqueryd.INFO.20200117-042121.1011"))
    info_paths.append(os.path.join(log_path, "osqueryd.INFO.20200117-042121.1012"))

    for path in info_paths:
        with open(path, 'a')as file:
            file.write("blah")

    edr_plugin_instance.start_edr()

    assert edr_plugin_instance.wait_log_contains("Removed old osquery INFO file:")

    preserved_files = []
    end_paths = os.listdir(log_path)
    for path in info_paths:
        if os.path.basename(path) in end_paths:
            print(path)
            preserved_files.append(path)

    assert len(preserved_files) == 10
    assert os.path.join(log_path, "osqueryd.INFO.20200117-042121.1012") in preserved_files
    removed_files = list(set(info_paths) - (set(preserved_files)))
    assert len(removed_files) == 2
    assert os.path.join(log_path, "osqueryd.INFO.20200117-042121.1001") in removed_files

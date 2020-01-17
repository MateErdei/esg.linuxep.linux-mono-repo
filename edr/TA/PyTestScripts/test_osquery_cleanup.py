import os
import time

def test_edr_plugin_purges_database_when_threshold_reached(sspl_mock, edr_plugin_instance):
    edr_plugin_instance.start_edr()
    i = 105
    time.sleep(3)
    while i > 0:
        db_path = edr_plugin_instance.osquery_database_path()
        path = os.path.join(db_path, str(i))
        with open(path, 'a')as file:
            file.write("blah")
        i -= 1

    edr_plugin_instance.stop_edr()
    edr_plugin_instance.start_edr()

    assert edr_plugin_instance.wait_log_contains("Purging Database")
    assert edr_plugin_instance.wait_log_contains("Purging Done", 15)
    files = os.listdir(db_path)
    assert len(files) < 10


def test_edr_plugin_rotates_logfiles_when_threshold_reached(sspl_mock, edr_plugin_instance):
    edr_plugin_instance.start_edr()
    i = 10
    time.sleep(3)
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

    edr_plugin_instance.stop_edr()
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


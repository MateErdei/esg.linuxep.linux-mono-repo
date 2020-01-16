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

def test_edr_plugin_rotates_logfiles_when_threshold_reached(sspl_mock, edr_plugin_instance):
    edr_plugin_instance.start_edr()
    i = 10
    time.sleep(3)
    while i > 0:
        log_path = edr_plugin_instance.log_path()
        path = os.path.join(log_path, "osqueryd.results.log." + str(i))
        with open(path, 'a')as file:
            file.write("blah")
        i -= 1

    file_to_rotate = os.path.join(log_path, "osqueryd.results.log")
    i = 1024*1024 +10

    with open(file_to_rotate, 'ab')as file:
        file.write(b"\0"*i)

    edr_plugin_instance.stop_edr()
    edr_plugin_instance.start_edr()

    assert edr_plugin_instance.wait_log_contains("Rotating osquery logs")
    assert edr_plugin_instance.wait_log_contains("Log limit reached : Deleting oldest osquery log file")
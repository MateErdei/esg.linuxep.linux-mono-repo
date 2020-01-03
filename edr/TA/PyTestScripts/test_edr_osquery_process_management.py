import psutil
import time
import os


def _wait_for_osquery_to_run():
    times_run = 0
    while times_run < 10:
        times_run += 1
        for p in psutil.process_iter():
            if p.name() == "osqueryd":
                return p.pid
        time.sleep(1)
    raise AssertionError("osqueryd not found in process list.")


def _wait_for_osquery_to_stop(pid):
    times_run = 0
    while times_run < 10:
        times_run += 1
        pids = []
        for p in psutil.process_iter():
            pids.append(p.pid)

        if pid in pids:
            time.sleep(1)
            continue
        return

    raise AssertionError("osqueryd failed to stop")


def test_edr_plugin_starts_osquery(sspl_mock, edr_plugin_instance):
    edr_plugin_instance.start_edr()
    _wait_for_osquery_to_run()


def test_edr_plugin_restarts_osquery_after_kill_signal(sspl_mock, edr_plugin_instance):
    edr_plugin_instance.start_edr()
    initial_osquery_pid = _wait_for_osquery_to_run()
    os.kill(initial_osquery_pid, 9)
    _wait_for_osquery_to_stop(initial_osquery_pid)
    new_osquery_pid = _wait_for_osquery_to_run()
    assert (initial_osquery_pid != new_osquery_pid)


def test_edr_plugin_restarts_osquery_after_segv_signal(sspl_mock, edr_plugin_instance):
    edr_plugin_instance.start_edr()
    initial_osquery_pid = _wait_for_osquery_to_run()
    os.kill(initial_osquery_pid, 11)
    _wait_for_osquery_to_stop(initial_osquery_pid)
    new_osquery_pid = _wait_for_osquery_to_run()
    assert (initial_osquery_pid != new_osquery_pid)


def test_osquery_is_stopped_if_edr_plugin_is_stopped(sspl_mock, edr_plugin_instance):
    edr_plugin_instance.start_edr()
    initial_osquery_pid = _wait_for_osquery_to_run()
    edr_plugin_instance.stop_edr()
    _wait_for_osquery_to_stop(initial_osquery_pid)


def test_edr_plugin_kills_existing_osquery_instances(sspl_mock, edr_plugin_instance):
    edr_plugin_instance.start_edr()
    initial_osquery_pid = _wait_for_osquery_to_run()
    edr_plugin_instance.kill_edr()
    edr_plugin_instance.start_edr()
    _wait_for_osquery_to_stop(initial_osquery_pid)
    new_osquery_pid = _wait_for_osquery_to_run()
    assert (initial_osquery_pid != new_osquery_pid)


def test_edr_plugin_regenerates_flags_file(sspl_mock, edr_plugin_instance):
    edr_plugin_instance.start_edr()
    initial_osquery_pid = _wait_for_osquery_to_run()
    edr_plugin_instance.stop_edr()
    _wait_for_osquery_to_stop(initial_osquery_pid)

    flags_file_path = os.path.join(os.environ['SOPHOS_INSTALL'], 'plugins/edr/etc/osquery.flags')

    # Quick sanity check to ensure the file has something we expect in it.
    with open(flags_file_path, 'r') as flags_file:
        # Check that something we expect is in the file which means the bad contents from above was overwritten
        assert ("host_identifier" in flags_file.read())

    # Replace flags file with rubbish text
    os.remove(flags_file_path)
    with open(flags_file_path, 'w') as flags_file:
        flags_file.write("bad content")

    # Start EDR again so it re-generates the flags file.
    edr_plugin_instance.start_edr()
    _wait_for_osquery_to_run()

    # Allow some time for the flags file to be overwritten
    tries = 5
    flags_file_was_regenerated = False
    while tries > 0:
        tries -= 1
        with open(flags_file_path, 'r') as flags_file:
            # Check that something we expect is in the file which means the bad contents from above was overwritten
            if "host_identifier" not in flags_file.read():
                time.sleep(1)
                continue
            else:
                flags_file_was_regenerated = True
    assert flags_file_was_regenerated





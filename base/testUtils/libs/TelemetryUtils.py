#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2019 Sophos Plc, Oxford, England.
# All rights reserved.


import json
import os
import pwd
import shutil
import time

import robot.api.logger as logger
from robot.libraries.BuiltIn import BuiltIn, RobotNotRunningError

import BaseInfo as base_info
import PathManager
import SystemInfo as system_info
from MDRUtils import get_mtr_version
from PluginUtils import get_plugin_version


class TelemetryUtils:
    def __init__(self):
        self._broken_commands = list()
        self._moved_suffix = "_moved"
        self._default_telemetry_config = {
            "telemetryServerCertificatePath": "",
            "externalProcessWaitRetries": 10,
            "externalProcessWaitTime": 100,
            "additionalHeaders": ["x-amz-acl: bucket-owner-full-control"],
            "maxJsonSize": 100000,
            "messageRelays": [],
            "port": 443,
            "proxies": [],
            "resourcePath": "linux/dev",
            "server": "localhost",
            "verb": "PUT"
        }

        self.telemetry_status_filepath = os.path.join(base_info.get_install(), 'base/telemetry/var/tscheduler-status.json')
        self.telemetry_supplementary_filepath = os.path.join(base_info.get_install(), 'base/etc/telemetry-config.json')
        self.telemetry_exe_config_filepath = os.path.join(base_info.get_install(), 'base/telemetry/var/telemetry-exe.json')

    def ordered(self, obj):
        if isinstance(obj, dict):
            return sorted((k, self.ordered(v)) for k, v in obj.items())
        if isinstance(obj, list):
            return sorted(self.ordered(x) for x in obj)
        else:
            return obj

    def generate_system_telemetry_dict(self,MCSProxy="Direct"):
        def update_system_telemetry_dict(telemetry_dict, key, getfunc):
            try:
                value = getfunc()
                if value is not None:
                    telemetry_dict[key] = value
            except Exception as getTelemetryExcept:
                logger.warn("Failed to get expected telemetry value for key : {}. Error: {}".format(key, str(
                    getTelemetryExcept)))

        telemetry = dict()
        update_system_telemetry_dict(telemetry, "cpu-cores", system_info.get_number_of_cpu_cores),
        update_system_telemetry_dict(telemetry, "disks", system_info.get_fstypes_and_free_space),
        update_system_telemetry_dict(telemetry, "kernel", system_info.get_kernel_version),
        update_system_telemetry_dict(telemetry, "locale", system_info.get_locale),
        update_system_telemetry_dict(telemetry, "memory-total", system_info.get_memory_total),
        update_system_telemetry_dict(telemetry, "os-name", system_info.get_os_name),
        update_system_telemetry_dict(telemetry, "os-version", system_info.get_os_version),
        update_system_telemetry_dict(telemetry, "uptime", system_info.get_up_time_seconds),
        update_system_telemetry_dict(telemetry, "timezone", system_info.get_abbreviated_timezone)
        update_system_telemetry_dict(telemetry, "selinux", system_info.get_selinux_status)
        update_system_telemetry_dict(telemetry, "apparmor", system_info.get_apparmor_status)
        update_system_telemetry_dict(telemetry, "auditd", system_info.get_auditd_status)
        telemetry["mcs-connection"] = MCSProxy
        return telemetry

    def generate_watchdog_telemetry_dict(self, expected_times):
        telemetry = {
            "health":0,
            "commscomponent-unexpected-restarts": int(expected_times),
            "managementagent-unexpected-restarts": int(expected_times),
            "sdu-unexpected-restarts": int(expected_times),
            "mcsrouter-unexpected-restarts": int(expected_times),
            "tscheduler-unexpected-restarts": int(expected_times),
            "updatescheduler-unexpected-restarts": int(expected_times)
        }
        return telemetry

    def generate_watchdog_telemetry_with_plugin(self, plugin_name, expected_times=0):
        telemetry = self.generate_watchdog_telemetry_dict(expected_times)
        telemetry[f"{plugin_name}-unexpected-restarts"] = int(expected_times)
        return telemetry

    def generate_system_telemetry_json(self):
        return json.dumps(self.generate_system_telemetry_dict())

    def generate_base_telemetry_dict(self):
        machine_id = base_info.get_machine_id_or_default()
        customer_id = base_info.get_customer_id_or_default()
        endpoint_id = base_info.get_endpoint_id_or_default()
        tenant_id = base_info.get_tenant_id_or_default()
        device_id = base_info.get_device_id_or_default()
        version = base_info.get_base_version_or_default()
        overall_health = base_info.get_base_overall_health_or_default()

        telemetry = {}

        if machine_id is not None:
            telemetry["machineId"] = machine_id

        if customer_id is not None:
            telemetry["customerId"] = customer_id

        if endpoint_id is not None:
            telemetry["endpointId"] = endpoint_id

        if tenant_id is not None:
            telemetry["tenantId"] = tenant_id

        if device_id is not None:
            telemetry["deviceId"] = device_id

        if version is not None:
            telemetry["version"] = version

        if overall_health is not None:
            telemetry["overall-health"] = overall_health

        return telemetry

    def generate_mtr_telemetry_dict(self, num_SophosMTR_restarts):
        version = get_mtr_version()
        telemetry = {
            "sophosmtr-restarts": int(num_SophosMTR_restarts),
            "version": version
        }

        return telemetry

    def generate_edr_telemetry_dict(self, num_osquery_restarts, num_database_purges, num_osquery_restarts_cpu,
                                    num_osquery_restarts_memory,
                                    num_extension_restarts,
                                    num_extension_restarts_cpu,
                                    num_extension_restarts_memory,
                                    xdr_is_enabled,
                                    events_max,
                                    failed_count,
                                    successful_count,
                                    folded_query):
        version = get_plugin_version("edr")
        telemetry = {
            "osquery-restarts": int(num_osquery_restarts),
            "mtr-extension-restarts": int(num_extension_restarts),
            "mtr-extension-restarts-cpu": int(num_extension_restarts_cpu),
            "mtr-extension-restarts-memory": int(num_extension_restarts_memory),
            "version": version,
            "events-max": events_max,
            "osquery-database-purges": int(num_database_purges),
            "xdr-is-enabled": xdr_is_enabled,
            "reached-max-process-events" : True,
            "reached-max-selinux-events" : True,
            "reached-max-socket-events" : True,
            "reached-max-user-events" : True
        }
        if int(num_osquery_restarts_cpu) > -1:
            telemetry["osquery-restarts-cpu"] = int(num_osquery_restarts_cpu)
        if int(num_osquery_restarts_memory) > -1:
            telemetry["osquery-restarts-memory"] = int(num_osquery_restarts_memory)
        if folded_query:
            telemetry['foldable-queries'] = ['running_processes_windows_sophos', 'stopped_processes_windows_sophos']

        telemetry["scheduled-queries"] = {}
        telemetry["scheduled-queries"]['upload-limit-hit'] = False

        if failed_count:
            telemetry["live-query"] = {"failed-count" : int(failed_count)}
        if successful_count:
            telemetry["live-query"] = {"successful-count" : int(successful_count)}

        return telemetry

    def generate_base_telemetry_json(self):
        return json.dumps(self.generate_base_telemetry_dict())

    def generate_update_scheduler_telemetry(self, number_failed_updates, most_recent_update_successful,
                                            successful_update_time, base_fixed_version, base_tag,
                                            mtr_fixed_version, mtr_tag, sddsid,set_edr,set_av,
                                            install_state,download_state, sdds_mechanism):
        health = 0
        if not(download_state == 0 and install_state == 0):
            health = 1
        telemetry = {
            "failed-update-count": int(number_failed_updates),
            "failed-downloader-count": 0,
            "install-state": install_state,
            "download-state": download_state,
            "health": health
        }

        if most_recent_update_successful is not None:
            telemetry["latest-update-succeeded"] = True if most_recent_update_successful == "True" else False

        if successful_update_time is not None:
            telemetry["successful-update-time"] = int(successful_update_time)

        subscriptions = [{"fixedversion": base_fixed_version, "tag": base_tag, "rigidname": "ServerProtectionLinux-Base"},
                         {"fixedversion": mtr_fixed_version, "tag": mtr_tag, "rigidname": "ServerProtectionLinux-Plugin-MDR"}]
        if set_edr:
            edr = {"fixedversion": "", "tag": "RECOMMENDED", "rigidname": "ServerProtectionLinux-Plugin-EDR"}
            subscriptions.append(edr)

        if set_av:
            av = {"fixedversion": "", "tag": "RECOMMENDED", "rigidname": "ServerProtectionLinux-Plugin-AV"}
            subscriptions.append(av)
        warehouse = {"sddsid": sddsid, "subscriptions": subscriptions}

        telemetry["warehouse"] = warehouse

        if sdds_mechanism:
            telemetry['sdds-mechanism'] = sdds_mechanism

        return telemetry

    def generate_liveresponse_telemetry_dict(self, total_sessions, failed_sessions, connection_data_keys):
        version = get_plugin_version("liveresponse")
        telemetry = {
            "version": version,
            "health": 0,
            "total-sessions": total_sessions,
            "failed-sessions": failed_sessions
        }
        if connection_data_keys:
            key_dict = {}
            for key in connection_data_keys:
                if key[0]:
                    key_dict[key[0]] = int(key[1])
            telemetry["session-connection-data"] = key_dict
        return telemetry

    def disks_mismatched(self, expected_disks, actual_disks):
        expected_fstypes = [mount["fstype"] for mount in expected_disks]
        actual_fstypes = [mount["fstype"] for mount in actual_disks]
        # only comparing filesystem types as free space changes
        return expected_fstypes != actual_fstypes

    def check_system_telemetry_json_is_correct(self, json_string, missing_key=None,MCSProxy="Direct"):
        expected_system_telemetry_dict = self.generate_system_telemetry_dict(MCSProxy)
        actual_system_telemetry_dict = json.loads(json_string)["system-telemetry"]

        # an extra tmpfs block can appear in the actual, pop it and compare the rest
        if len(actual_system_telemetry_dict.get("disks", list())) > len(
                expected_system_telemetry_dict.get("disks", list())):
            _ = actual_system_telemetry_dict["disks"].pop()
        # ignore cloud platform details
        actual_system_telemetry_dict.pop("cloud-platform", None)
        self.check_system_telemetry_is_correct(actual_system_telemetry_dict, expected_system_telemetry_dict,
                                               missing_key)

    def check_system_telemetry_is_correct(self, actual_dict, expected_dict, missing_key):
        if missing_key is not None:
            assert (actual_dict.pop(missing_key, None) is None)
            expected_missing_key = expected_dict.pop(missing_key)

        if missing_key != "uptime":
            expected_uptime = expected_dict.pop("uptime")
            actual_uptime = actual_dict.pop("uptime")

            if abs(actual_uptime - expected_uptime) > 60:
                raise AssertionError("System telemetry generated by product doesn't match telemetry expected by test.\n"
                                     "Expected uptime: {}\nActual uptime: {}".format(expected_uptime, actual_uptime))
        if missing_key != "disks":
            expected_disks = expected_dict.pop("disks")
            actual_disks = actual_dict.pop("disks")

            if self.disks_mismatched(expected_disks, actual_disks):
                raise AssertionError("System telemetry generated by product doesn't match telemetry expected by test.\n"
                                     "Expected disks: {}\nActual disks: {}".format(expected_disks, actual_disks))

            if expected_dict != actual_dict:
                raise AssertionError("System telemetry generated by product doesn't match telemetry expected by test.\n"
                                     "Expected telemetry: {}\nActual telemetry: {}".format(expected_dict, actual_dict))

    def check_watchdog_telemetry_json_is_correct(self, json_string, expected_times=0, plugin_name=None):
        if plugin_name:
            expected_watchdog_telemetry_dict = self.generate_watchdog_telemetry_with_plugin(plugin_name, expected_times)
        else:
            expected_watchdog_telemetry_dict = self.generate_watchdog_telemetry_dict(expected_times)

        actual_watchdog_telemetry_dict = json.loads(json_string)["watchdogservice"]

        #pop mcsrouter before comparison
        actual_watchdog_telemetry_dict.pop("mcsrouter-unexpected-restarts")
        expected_watchdog_telemetry_dict.pop("mcsrouter-unexpected-restarts")
        self.check_watchdog_telemetry_is_correct(actual_watchdog_telemetry_dict, expected_watchdog_telemetry_dict)

    def check_watchdog_telemetry_is_correct(self, actual_dict, expected_dict):
        assert actual_dict == expected_dict, "actual watchdog telemetry: \n\t{}\n did not match expected: \n\t{}".format(
            actual_dict, expected_dict)

    def check_watchdog_saved_json_strings_are_equal(self, actual_json, expected_times=0, plugin_name=None):
        actual_dict = json.loads(actual_json)["rootkey"]
        if plugin_name:
            expected_dict = self.generate_watchdog_telemetry_with_plugin(expected_times, plugin_name)
        else:
            expected_dict = self.generate_watchdog_telemetry_dict(expected_times)
        #pop mcsrouter efore comparison
        actual_dict.pop("mcsrouter-unexpected-restarts")
        expected_dict.pop("mcsrouter-unexpected-restarts")
        expected_dict.pop("health")

        self.check_watchdog_telemetry_is_correct(actual_dict,expected_dict)

    def check_update_scheduler_telemetry_json_is_correct(self, json_string, number_failed_updates,
                                                         most_recent_update_successful=None,
                                                         successful_update_time=None, timing_tolerance=10,
                                                         base_fixed_version="", base_tag="RECOMMENDED",
                                                         mtr_fixed_version="", mtr_tag="RECOMMENDED", sddsid="",
                                                         set_edr=False, set_av=False, install_state=0,download_state=0,
                                                         sdds_mechanism=None):
        ignore_sdds_mechanism = sdds_mechanism is None
        expected_update_scheduler_telemetry_dict = self.generate_update_scheduler_telemetry(number_failed_updates,
                                                                                            most_recent_update_successful,
                                                                                            successful_update_time,
                                                                                            base_fixed_version, base_tag,
                                                                                            mtr_fixed_version, mtr_tag,
                                                                                            sddsid,set_edr,set_av,
                                                                                            install_state,download_state,
                                                                                            sdds_mechanism)
        actual_update_scheduler_telemetry_dict = json.loads(json_string)["updatescheduler"]

        self.check_update_scheduler_telemetry_is_correct(actual_update_scheduler_telemetry_dict,
                                                         expected_update_scheduler_telemetry_dict, timing_tolerance,
                                                         ignore_sdds_mechanism)

    def check_update_scheduler_telemetry_is_correct(self, actual_dict, expected_dict, timing_tolerance, ignore_sdds_mechanism):
        expected_successful_update_time = expected_dict.pop("successful-update-time", None)
        actual_successful_update_time = actual_dict.pop("successful-update-time", None)

        if ignore_sdds_mechanism:
            expected_dict.pop("sdds-mechanism", None)
            actual_dict.pop("sdds-mechanism", None)

        if expected_successful_update_time is not None and actual_successful_update_time is not None:
            time_difference = abs(expected_successful_update_time - actual_successful_update_time)

            if time_difference > timing_tolerance:
                raise AssertionError(
                    "Update scheduler telemetry generated by product doesn't match telemetry expected by test.\n"
                    "Expected successful update time: {}\nActual successful update time: {}".format(
                        expected_successful_update_time, actual_successful_update_time))

        if self.ordered(expected_dict) != self.ordered(actual_dict):
            raise AssertionError(
                "Update scheduler telemetry generated by product doesn't match telemetry expected by test.\n"
                "Expected telemetry: {}\nActual telemetry: {}".format(expected_dict, actual_dict))

    def check_base_telemetry_json_is_correct(self, json_string, expected_missing_keys=None, ignored_fields=None):
        expected_base_telemetry_dict = self.generate_base_telemetry_dict()
        actual_base_telemetry_dict = json.loads(json_string)["base-telemetry"]
        self.check_base_telemetry_is_correct(actual_base_telemetry_dict, expected_base_telemetry_dict,
                                             expected_missing_keys, ignored_fields)

    def check_base_telemetry_is_correct(self, actual_dict, expected_dict, expected_missing_keys=None, ignored_fields=None):
        # check key is there before popping
        if expected_missing_keys is not None:
            if isinstance(expected_missing_keys, list):
                for key in expected_missing_keys:
                    if key in expected_dict:
                        expected_dict.pop(key)
                    logger.info("Popped key from expected: {}".format(key))
            elif isinstance(expected_missing_keys, str):
                if expected_missing_keys in expected_dict:
                    expected_dict.pop(expected_missing_keys)
                logger.info("Popped key from expected: {}".format(expected_missing_keys))

        if ignored_fields:
            if isinstance(ignored_fields, list):
                for key in ignored_fields:
                    logger.info(f"Ignoring key: {key}")
                    if key in expected_dict:
                        expected_dict.pop(key)
                    if key in actual_dict:
                        actual_dict.pop(key)
            elif isinstance(ignored_fields, str):
                logger.info(f"Ignoring key: {ignored_fields}")
                if ignored_fields in expected_dict:
                    expected_dict.pop(ignored_fields)
                if ignored_fields in actual_dict:
                    actual_dict.pop(ignored_fields)

        if actual_dict != expected_dict:
            raise AssertionError(
                "Base telemetry generated by product doesn't match telemetry expected by test. expected: {}, actual: {}".format(
                    expected_dict, actual_dict))

    def check_mtr_telemetry_json_is_correct(self, json_string,
                                            num_sophos_mtr_restarts=0,
                                            ignore_cpu_restarts=False,
                                            ignore_memory_restarts=False,
                                            ignore_health=True):
        expected_mtr_telemetry_dict = self.generate_mtr_telemetry_dict(num_sophos_mtr_restarts)
        actual_mtr_telemetry_dict = json.loads(json_string)["mtr"]

        if ignore_cpu_restarts:
            cpu_restarts_key = "osquery-restarts-cpu"
            expected_mtr_telemetry_dict.pop(cpu_restarts_key, None)
            actual_mtr_telemetry_dict.pop(cpu_restarts_key, None)

        if ignore_memory_restarts:
            mem_restarts_key = "osquery-restarts-memory"
            expected_mtr_telemetry_dict.pop(mem_restarts_key, None)
            actual_mtr_telemetry_dict.pop(mem_restarts_key, None)

        if ignore_health:
            health_key = "health"
            expected_mtr_telemetry_dict.pop(health_key, None)
            actual_mtr_telemetry_dict.pop(health_key, None)

        if actual_mtr_telemetry_dict != expected_mtr_telemetry_dict:
            raise AssertionError(
                "MTR telemetry doesn't match telemetry expected by test. expected: {}, actual: {}".format(
                    expected_mtr_telemetry_dict, actual_mtr_telemetry_dict))

    def check_edr_telemetry_json_is_correct(self, json_string,
                                            num_osquery_restarts=0,
                                            num_database_purges=0,
                                            num_osquery_restarts_cpu=0,
                                            num_osquery_restarts_memory=0,
                                            num_extension_restarts=0,
                                            num_extension_restarts_cpu=0,
                                            num_extension_restarts_memory=0,
                                            xdr_is_enabled=False,
                                            events_max='100000',
                                            ignore_cpu_restarts=False,
                                            ignore_memory_restarts=False,
                                            ignore_mtr_extension_restarts=True,
                                            ignore_scheduled_queries=True,
                                            failed_count=None,
                                            successful_count=None,
                                            ignore_xdr=True,
                                            ignore_process_events=True,
                                            ignore_selinux_events=True,
                                            ignore_socket_events=True,
                                            ignore_user_events=True,
                                            folded_query=False):
        expected_edr_telemetry_dict = self.generate_edr_telemetry_dict(num_osquery_restarts,
                                                                       num_database_purges,
                                                                       num_osquery_restarts_cpu,
                                                                       num_osquery_restarts_memory,
                                                                       num_extension_restarts,
                                                                       num_extension_restarts_cpu,
                                                                       num_extension_restarts_memory,
                                                                       xdr_is_enabled,
                                                                       events_max,
                                                                       failed_count,
                                                                       successful_count,
                                                                       folded_query)
        actual_edr_telemetry_dict = json.loads(json_string)["edr"]
        actual_edr_telemetry_dict.pop("health", None)
        if ignore_cpu_restarts:
            cpu_restarts_key = "osquery-restarts-cpu"
            expected_edr_telemetry_dict.pop(cpu_restarts_key, None)
            actual_edr_telemetry_dict.pop(cpu_restarts_key, None)

        if ignore_memory_restarts:
            mem_restarts_key = "osquery-restarts-memory"
            expected_edr_telemetry_dict.pop(mem_restarts_key, None)
            actual_edr_telemetry_dict.pop(mem_restarts_key, None)

        if ignore_scheduled_queries:
            scheduled_queries_key = "scheduled-queries"
            expected_edr_telemetry_dict.pop(scheduled_queries_key, None)
            actual_edr_telemetry_dict.pop(scheduled_queries_key, None)

        if ignore_mtr_extension_restarts:
            # OSQuery 5.0.1 has changed logging, so we cannot directly tell when the mtr extension stops
            # All we can do is tell when os query starts the mtr extension.
            mtr_extension_restarts_key = "mtr-extension-restarts"
            expected_edr_telemetry_dict.pop(mtr_extension_restarts_key, None)
            actual_edr_telemetry_dict.pop(mtr_extension_restarts_key, None)
        osquery_db_size_key = "osquery-database-size"
        if actual_edr_telemetry_dict[osquery_db_size_key] < 1:
            raise AssertionError("EDR telemetry doesn't contain a valid osquery database size field")
        actual_edr_telemetry_dict.pop(osquery_db_size_key, None)

        if ignore_xdr:
            xdr_key = "xdr-is-enabled"
            expected_edr_telemetry_dict.pop(xdr_key, None)
            actual_edr_telemetry_dict.pop(xdr_key, None)

        if ignore_process_events:
            process_events_key = "reached-max-process-events"
            expected_edr_telemetry_dict.pop(process_events_key, None)
            actual_edr_telemetry_dict.pop(process_events_key, None)

        if ignore_selinux_events:
            selinux_events_key = "reached-max-selinux-events"
            expected_edr_telemetry_dict.pop(selinux_events_key, None)
            actual_edr_telemetry_dict.pop(selinux_events_key, None)

        if ignore_socket_events:
            socket_events_key = "reached-max-socket-events"
            expected_edr_telemetry_dict.pop(socket_events_key, None)
            actual_edr_telemetry_dict.pop(socket_events_key, None)

        if ignore_user_events:
            user_events_key = "reached-max-user-events"
            expected_edr_telemetry_dict.pop(user_events_key, None)
            actual_edr_telemetry_dict.pop(user_events_key, None)

        if actual_edr_telemetry_dict != expected_edr_telemetry_dict:
            raise AssertionError(
                "EDR telemetry doesn't match telemetry expected by test. expected: {}, actual: {}".format(
                    expected_edr_telemetry_dict, actual_edr_telemetry_dict))

    def check_liveresponse_telemetry_json_is_correct(self,
                                                     json_string,
                                                     total_sessions=0,
                                                     failed_sessions=0,
                                                     connection_data_keys=None
                                                     ):
        expected_lr_telemetry_dict = self.generate_liveresponse_telemetry_dict(
                                        total_sessions, failed_sessions, connection_data_keys)
        actual_lr_telemetry_dict = json.loads(json_string)["liveresponse"]

        if actual_lr_telemetry_dict != expected_lr_telemetry_dict:
            raise AssertionError(
                "LiveResponse telemetry doesn't match telemetry expected by test. expected: {}, actual: {}".format(
                    expected_lr_telemetry_dict, actual_lr_telemetry_dict))

    def move_file_from_expected_path(self, command_path):
        new_command_path = command_path + self._moved_suffix
        if os.path.exists(command_path):
            shutil.move(command_path, new_command_path)
        self._broken_commands.append(command_path)

    def restore_moved_files(self):
        while len(self._broken_commands) > 0:
            command_path = self._broken_commands.pop()
            moved_command_path = command_path + self._moved_suffix
            if os.path.exists(command_path):
                os.remove(command_path)
            if os.path.exists(moved_command_path):
                shutil.move(moved_command_path, command_path)

    def break_system_command(self, command_path):
        self.move_file_from_expected_path(command_path)

    def restore_system_commands(self):
        self.restore_moved_files()

    def hang_system_command(self, command_path):
        self.break_system_command(command_path)
        hang_file = os.path.abspath(os.path.join(PathManager.get_support_file_path(), "Telemetry/hang"))
        if not os.path.exists(hang_file):
            raise AssertionError("Test file : {} was not in the support files".format(hang_file))
        shutil.copyfile(hang_file, command_path)
        os.chmod(command_path, 0o777)

    def overload_system_command(self, command_path):
        self.break_system_command(command_path)
        overload_file = os.path.abspath(os.path.join(PathManager.get_support_file_path(), "Telemetry/overload_stdout"))
        if not os.path.exists(overload_file):
            raise AssertionError("Test file : {} was not in the support files".format(overload_file))
        shutil.copyfile(overload_file, command_path)
        os.chmod(command_path, 0o777)

    def crash_system_command(self, command_path):
        self.break_system_command(command_path)
        crash_file = os.path.abspath(os.path.join(PathManager.get_support_file_path(), "Telemetry/crash"))
        if not os.path.exists(crash_file):
            raise AssertionError("Test file : {} was not in the support files".format(crash_file))
        shutil.copyfile(crash_file, command_path)
        os.chmod(command_path, 0o777)

    def permission_denied_system_command(self, command_path):
        self.break_system_command(command_path)
        root_file = os.path.abspath(os.path.join(PathManager.get_support_file_path(), "Telemetry/hang"))
        if not os.path.exists(root_file):
            raise AssertionError("Test file : {} was not in the support files".format(root_file))
        shutil.copyfile(root_file, command_path)
        os.chmod(command_path, 0o744)

    def create_test_telemetry_config_file(self, telemetry_config_file_path, certificate_path="", username="sophos-spl-user",
                                          requestType="PUT", port=443, server=None):
        logger.info(certificate_path)
        if certificate_path.strip('"\''):
            fixedTelemetryCert="/opt/sophos-spl/base/mcs/certs/telemetry.crt"
            shutil.copyfile(certificate_path, fixedTelemetryCert)
            os.chmod(fixedTelemetryCert, 0o644)
            self._default_telemetry_config["telemetryServerCertificatePath"] = fixedTelemetryCert
        else:
            self._default_telemetry_config["telemetryServerCertificatePath"] = ""

        if server:
            self._default_telemetry_config["server"] = server
        self._default_telemetry_config["verb"] = requestType
        self._default_telemetry_config["port"] = int(port)
        with open(telemetry_config_file_path, 'w') as tcf:
            tcf.write(json.dumps(self._default_telemetry_config))
        self.chown(telemetry_config_file_path, username)

    def set_telemetry_config_jsonMax(self, jsonmax):
        self._default_telemetry_config["maxJsonSize"] = int(jsonmax.strip())

    def create_broken_telemetry_config_file(self, telemetry_config_file_path, username):
        with open(telemetry_config_file_path, 'w') as tcf:
            tcf.write("{rubbish ; json , string to break. parsing of config:[file],}")
        self.chown(telemetry_config_file_path, username)

    def chown(self, file_path, username):
        uid = pwd.getpwnam(username).pw_uid
        os.chown(file_path, uid, -1)

    def get_install(self):
        try:
            return format(BuiltIn().get_variable_value("${SOPHOS_INSTALL}"))
        except RobotNotRunningError:
            try:
                return os.environ.get("SOPHOS_INSTALL", "/opt/sophos-spl")
            except KeyError:
                return "/opt/sophos-spl"

    def check_status_file_is_generated_and_return_scheduled_time(self, wait_time_seconds=20):
        """Wait for up to wait_time_seconds for status file to exist, then wait up to another wait_time_seconds for expected JSON content."""

        self.wait_for_file_to_exist(self.telemetry_status_filepath, wait_time_seconds)

        wait_time_seconds_int = int(wait_time_seconds)

        for i in range(wait_time_seconds_int):
            try:
                scheduled_time = self.get_scheduled_time_from_scheduler_status_file()
                return scheduled_time
            except KeyError:
                time.sleep(1)

        raise AssertionError(
            "Scheduled time not found in: {} within the given time limit".format(self.telemetry_status_filepath))

    def get_interval_from_supplementary_file(self):
        if os.path.exists(self.telemetry_supplementary_filepath):
            with open(self.telemetry_supplementary_filepath) as supplFile:
                return json.loads(supplFile.read())['interval']

        raise AssertionError(
            "Telemetry supplementary file not present or does not have expected key: " + self.telemetry_supplementary_filepath)

    def set_interval_in_configuration_file(self, new_interval_in_seconds=10):
        if os.path.exists(self.telemetry_supplementary_filepath):
            with open(self.telemetry_supplementary_filepath, "r+") as supplFile:
                config = json.loads(supplFile.read())

                config['interval'] = int(new_interval_in_seconds)
                config["additionalHeaders"] = ["x-amz-acl:bucket-owner-full-control"]
                supplFile.seek(0)
                supplFile.write(json.dumps(config))
                supplFile.truncate()

    def verify_scheduled_time_is_expected(self, scheduled_time, expected_time, seconds_tolerance=30):
        scheduled = int(scheduled_time)
        expected = int(expected_time)
        tolerance = int(seconds_tolerance)
        return scheduled >= (expected - tolerance) and scheduled <= (expected + tolerance)

    def expected_scheduled_time(self, start_time, interval):
        return int(start_time) + int(interval)

    def wait_for_file_to_exist(self, filepath, wait_time_seconds=20):
        """Wait for up to wait_time_seconds seconds for filepath to exist."""
        wait_time_seconds_int = int(wait_time_seconds)

        dir = os.path.dirname(filepath)
        filename = os.path.basename(filepath)
        for i in range(wait_time_seconds_int):
            l = os.listdir(dir)
            if filename in l:
                return
            time.sleep(1)

        raise AssertionError(
            "Expected file: {} was not created in dir: {} within the given time limit".format(filename, dir))

    def get_scheduled_time_from_scheduler_status_file(self):
        with open(self.telemetry_status_filepath) as status_file:
            status_file_contents = status_file.read()
            return json.loads(status_file_contents)['scheduled-time']

    def set_scheduled_time_in_scheduler_status_file(self, schedule_time):
        with open(self.telemetry_status_filepath, "w") as statusFile:
            config = {"scheduled-time": int(schedule_time)}
            statusFile.write(json.dumps(config))

    def break_scheduler_status_file(self):
        with open(self.telemetry_status_filepath, "w") as statusFile:
            config = {"scheduled-time-Broken": ""}
            statusFile.write(json.dumps(config))

    def set_invalid_additional_headers_in_configuration_file(self):
        if os.path.exists(self.telemetry_supplementary_filepath):
            with open(self.telemetry_supplementary_filepath, "r+") as supplFile:
                config = json.loads(supplFile.read())

                config["additionalHeaders"] = ["x-amz-acl:not-really-valid"]
                supplFile.seek(0)
                supplFile.write(json.dumps(config))
                supplFile.truncate()

    def set_invalid_json_in_configuration_file(self):
        if os.path.exists(self.telemetry_supplementary_filepath):
            with open(self.telemetry_supplementary_filepath, "w") as supplFile:
                supplFile.write("{rubbish ; json , string to break. parsing of supplementary:[file],}")

    def get_current_time(self):
        return int(time.time())

    def get_syslog_location(self):
        locations = ["/var/log/syslog", "/var/log/messages"]
        for location in locations:
            if os.path.exists(location):
                return location
        raise AssertionError(
            "Syslog filew not found in expected locations: {}".format(locations))

    def check_if_we_expect_telemetry_for_restarts(self, path):
        if os.path.exists(path):
            return 0
        return -1

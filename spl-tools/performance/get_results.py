# This script merges data from two elastic search indexes and saves the result to a MySQL DB which we use for querying.
# This should be run at least daily on any machine, currently setup to run on sspl-perf-mon.
# the data is made up of two sections, the metric data gathered on performance machines using metricbeat and a second
# section of data which is entered in to elastic search by curl commands from performance scripts on the machines.
# The second set of data (custom data) contains certain specific things about the perf tests,
# e.g. duration, start time and end time.
import traceback
from datetime import datetime
from elasticsearch import Elasticsearch
import sys

if sys.version_info[0] < 3:
    raise Exception("Must be using Python 3")

try:
    import mysql.connector
except:
    print("No module named 'mysql', please install it: python3 -m pip install mysql-connector-python")
    exit()


def create_task(task_row, es):
    task = {}

    if "datetime" not in task_row:
        return None

    if "start" not in task_row:
        return None

    if "finish" not in task_row:
        return None

    if "eventname" not in task_row:
        return None

    # base version and build date
    if "product_version" not in task_row:
        return None

    if "build_date" not in task_row:
        return None

    if not isinstance(task_row["start"], int):
        return None

    if not isinstance(task_row["finish"], int):
        return None

    task["name"] = task_row["eventname"]
    task["datetime"] = task_row["datetime"]
    task["start"] = task_row["start"]
    task["finish"] = task_row["finish"]
    task["hostname"] = task_row["hostname"]
    task["duration"] = task_row["duration"]

    # convert None to none string if needed, e.g. base or plugins may not be installed so the versions can be none
    keys = ["product_version", "build_date", "edr_product_version", "edr_build_date", "mtr_product_version", "mtr_build_date"]
    for key in keys:
        if key in task_row and task_row[key]:
            task[key] = task_row[key]
        else:
            task[key] = "none"

    # This can be uncommented if we wish to go back to averaging out results per day, see date_key.
    # task["day"] = task_row["datetime"].split(' ')[0]

    task["avg_cpu"], task["max_cpu"], task["avg_mem"], task["max_mem"], task["min_mem"] = get_metrics(task["hostname"],
                                                                                                      task["start"],
                                                                                                      task["finish"],
                                                                                                      es)
    return task


def get_metrics(hostname, from_timestamp, to_timestamp, es):
    from_datetime = datetime.utcfromtimestamp(from_timestamp)
    to_datetime = datetime.utcfromtimestamp(to_timestamp)

    metrics_index = "metric*"
    res_metrics = es.search(index=metrics_index,
                            body={
                                "query": {
                                    "match": {
                                        "agent.hostname": hostname
                                    }
                                },

                                "aggs": {
                                    "task_time_range": {
                                        "filter": {
                                            "range": {
                                                "@timestamp": {
                                                    "gte": from_datetime,
                                                    "lte": to_datetime
                                                }
                                            }
                                        },
                                        "aggs": {
                                            "avg_cpu": {
                                                "avg": {
                                                    "field": "system.cpu.total.pct"
                                                }
                                            },
                                            "max_cpu": {
                                                "max": {
                                                    "field": "system.cpu.total.pct"
                                                }
                                            },
                                            "avg_mem": {
                                                "avg": {
                                                    "field": "system.memory.used.bytes"
                                                }
                                            },
                                            "max_mem": {
                                                "max": {
                                                    "field": "system.memory.used.bytes"
                                                }
                                            },
                                            "min_mem": {
                                                "min": {
                                                    "field": "system.memory.used.bytes"
                                                }
                                            }
                                        },

                                    }
                                }
                            },
                            size=100,
                            request_timeout=60)

    avg_cpu = res_metrics["aggregations"]["task_time_range"]["avg_cpu"]["value"]
    max_cpu = res_metrics["aggregations"]["task_time_range"]["max_cpu"]["value"]
    avg_mem = res_metrics["aggregations"]["task_time_range"]["avg_mem"]["value"]
    max_mem = res_metrics["aggregations"]["task_time_range"]["max_mem"]["value"]
    min_mem = res_metrics["aggregations"]["task_time_range"]["min_mem"]["value"]

    return avg_cpu, max_cpu, avg_mem, max_mem, min_mem


def all_fields_present(to_check):
    fields = ['avg_cpu', 'avg_mem', 'max_cpu', 'max_mem', 'min_mem', 'duration', 'hostname']

    for field in fields:
        if field not in to_check:
            return False
        if to_check[field] is None:
            return False

    return True


def event_of_interest(event_name):
    # Any performance tasks / events that start with one of these prefixes will be included in the query.
    # These should correspond to the event names in the RunEDRPerfTests and in the run_tests.sh
    event_name_prefixes = ["build_gcc", "copy_files", "local-query_", "central-live-query_", "GCC"]
    for prefix in event_name_prefixes:
        if event_name.startswith(prefix):
            return True
    return False


def get_version_combo_id(task):
    if "product_version" not in task or "edr_product_version" not in task or "mtr_product_version" not in task:
        print(task)
        exit(1)
    return str(task["product_version"]) + "-" + str(task["edr_product_version"]) + "-" + str(task["mtr_product_version"])


def get_results_for_machine(hostname):
    es = Elasticsearch(["sspl-perf-mon"])
    perf_index = "perf-custom"
    es.indices.refresh(index=perf_index)

    res = es.search(
        index=perf_index,
        body={"query": {"match": {"hostname.keyword": hostname}}, "sort": [{"start": {"order": "desc"}}]},
        size=100,
        request_timeout=60)

    task_names = []
    prod_versions = []
    tasks = []
    days = []

    # This date_key was changed from "day", previously we would average out any tests of the same name that happened
    # within the same day. E.g. 2 tests in a day that built gcc on a machine would be averaged out in terms of
    # duration and cpu use etc. This was done using the day of the test (i.e. no time)
    # Instead we now use the dull date including the time which means that we generate a unique result per test per day
    # and do not average them
    date_key = "datetime"

    # Build up lists of task names, days etc.
    for hit in res['hits']['hits']:

        # Skip events / test loads we are not interested in.
        if not event_of_interest(hit["_source"]["eventname"]):
            print("Skipping: {}".format(hit["_source"]))
            continue

        task = create_task(hit["_source"], es)
        if not task:
            continue

        # Build list of tasks
        if hit["_source"]["eventname"] not in task_names:
            task_names.append(task["name"])

        # Build list of product version combinations, e.g. edr+mtr, edr, none
        version_combo_id = get_version_combo_id(task)
        if version_combo_id not in prod_versions:
            prod_versions.append(version_combo_id)

        # Build list of days
        if task[date_key] not in days:
            days.append(task[date_key])

        tasks.append(task)

    result_root = {}
    for day in days:
        result_root[day] = {}
        for product_version in prod_versions:
            result_root[day][product_version] = {}
            for task_name in task_names:
                result_root[day][product_version][task_name] = []
                for task in tasks:
                    if task[date_key] == day and task['name'] == task_name and get_version_combo_id(task) == product_version:
                        result_root[day][product_version][task_name].append(task)

    summary_root = {}
    for day in result_root:
        summary_root[day] = {}

        for version in result_root[day]:
            summary_root[day][version] = {}

            for task_name in result_root[day][version]:

                if len(result_root[day][version][task_name]) == 0:
                    continue

                summary_root[day][version][task_name] = {}
                good_result_count = 0
                summary_root[day][version][task_name]['avg_cpu'] = 0
                summary_root[day][version][task_name]['avg_mem'] = 0

                summary_root[day][version][task_name]['max_cpu'] = None
                summary_root[day][version][task_name]['max_mem'] = None
                summary_root[day][version][task_name]['min_mem'] = None

                for result in result_root[day][version][task_name]:

                    if all_fields_present(result):
                        summary_root[day][version][task_name]['duration'] = result['duration']
                        summary_root[day][version][task_name]['hostname'] = result['hostname']
                        summary_root[day][version][task_name]['base_version'] = result['product_version']
                        summary_root[day][version][task_name]['edr_version'] = result['edr_product_version']
                        summary_root[day][version][task_name]['mtr_version'] = result['mtr_product_version']
                        summary_root[day][version][task_name]['avg_cpu'] += result['avg_cpu']
                        summary_root[day][version][task_name]['avg_mem'] += result['avg_mem']

                        if summary_root[day][version][task_name]['max_cpu'] is None or result['max_cpu'] > \
                                summary_root[day][version][task_name]['max_cpu']:
                            summary_root[day][version][task_name]['max_cpu'] = result['max_cpu']

                        if summary_root[day][version][task_name]['max_mem'] is None or result['max_mem'] > \
                                summary_root[day][version][task_name]['max_mem']:
                            summary_root[day][version][task_name]['max_mem'] = result['max_mem']

                        if summary_root[day][version][task_name]['min_mem'] is None or result['min_mem'] < \
                                summary_root[day][version][task_name]['min_mem']:
                            summary_root[day][version][task_name]['min_mem'] = result['min_mem']

                        good_result_count += 1

                if good_result_count == 0:
                    continue

                summary_root[day][version][task_name]['avg_cpu'] /= good_result_count
                summary_root[day][version][task_name]['avg_mem'] /= good_result_count

                summary_root[day][version][task_name]['avg_cpu'] *= 100
                summary_root[day][version][task_name]['avg_cpu'] = "{0:.2f}".format(
                    round(summary_root[day][version][task_name]['avg_cpu'], 2))

                summary_root[day][version][task_name]['max_cpu'] *= 100
                summary_root[day][version][task_name]['max_cpu'] = "{0:.2f}".format(
                    round(summary_root[day][version][task_name]['max_cpu'], 2))

                summary_root[day][version][task_name]['avg_mem'] /= 1000000
                summary_root[day][version][task_name]['avg_mem'] = "{0:.0f}".format(
                    round(summary_root[day][version][task_name]['avg_mem'], 2))

                summary_root[day][version][task_name]['max_mem'] /= 1000000
                summary_root[day][version][task_name]['max_mem'] = "{0:.0f}".format(
                    round(summary_root[day][version][task_name]['max_mem'], 2))

                summary_root[day][version][task_name]['min_mem'] /= 1000000
                summary_root[day][version][task_name]['min_mem'] = "{0:.0f}".format(
                    round(summary_root[day][version][task_name]['min_mem'], 2))

    array_data = []

    for datetime_key, datetime_value in summary_root.items():
        for version_key, version_value in datetime_value.items():
            for test_key, test_values in version_value.items():
                row = {}
                row["Date"] = datetime_key
                row["Version"] = version_key
                row["Test"] = test_key
                for test_value_key, test_value in test_values.items():
                    row[test_value_key] = test_value
                array_data.append(row)

    performance_db = mysql.connector.connect(
        host="sspl-alex-1",
        user="inserter",
        passwd="insert",
        database="performance"
    )
    cursor = performance_db.cursor()

    for row in array_data:
        print("Inserting {}:".format(row))
        try:
            df_sql = "CALL update_perf_data(%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)"
            df_val = (
                row["Date"], row["base_version"], row["Test"], row["avg_cpu"], row["avg_mem"], row["max_cpu"],
                row["max_mem"], row["min_mem"], row["duration"], row["hostname"], row["edr_version"],
                row['mtr_version'])
            cursor.execute(df_sql, df_val)
            performance_db.commit()
        except Exception as ex:
            print("Exception for: {} ".format(row))
            print("Exception: {} ".format(ex))
            traceback.print_exc(file=sys.stdout)


# Start
performance_machines = ["sspl-perf-load", "sspl-perf-edr", "sspl-perf-edrmtr", "sspl-perf-stress"]
for machine in performance_machines:
    get_results_for_machine(machine)

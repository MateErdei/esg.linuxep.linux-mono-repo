import sys
if sys.version_info[0] < 3:
    raise Exception("Must be using Python 3")

from datetime import datetime
from elasticsearch import Elasticsearch
import json

def create_task(task_row):
    task = {}

    if "datetime" not in task_row:
        return None

    if "start" not in task_row:
        return None

    if "finish" not in task_row:
        return None

    if "eventname" not in task_row:
        return None

    if "product_version" not in task_row:
        return None

    if "build_date" not in task_row:
        return None

    task["name"] = task_row["eventname"]
    task["datetime"] = task_row["datetime"]
    task["start"] = task_row["start"]
    task["finish"] = task_row["finish"]
    task["hostname"] = task_row["hostname"]
    task["duration"] = task_row["duration"]
    task["product_version"] = task_row["product_version"]
    task["build_date"] = task_row["build_date"]
    task["day"] = task_row["datetime"].split(' ')[0]

    # return avg_cpu, max_cpu, avg_mem, max_mem, min_mem

    task["avg_cpu"], task["max_cpu"], task["avg_mem"], task["max_mem"], task["min_mem"] = get_metrics(task["hostname"], task["start"], task["finish"])

#    print(task)

    return task


def get_metrics(hostname, from_timestamp, to_timestamp):

    from_datetime = datetime.utcfromtimestamp(from_timestamp)
    to_datetime = datetime.utcfromtimestamp(to_timestamp)

    #print(from_datetime)
    #print(to_datetime)

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
                    size=100)


    avg_cpu = res_metrics["aggregations"]["task_time_range"]["avg_cpu"]["value"]
    max_cpu = res_metrics["aggregations"]["task_time_range"]["max_cpu"]["value"]
    avg_mem = res_metrics["aggregations"]["task_time_range"]["avg_mem"]["value"]
    max_mem = res_metrics["aggregations"]["task_time_range"]["max_mem"]["value"]
    min_mem = res_metrics["aggregations"]["task_time_range"]["min_mem"]["value"]

    return avg_cpu, max_cpu, avg_mem, max_mem, min_mem


def all_fields_present(to_check):
    if 'avg_cpu' not in to_check:
        return False
    if to_check['avg_cpu'] is None:
        return False

    if 'avg_mem' not in to_check:
        return False
    if to_check['avg_mem'] is None:
        return False

    if 'max_cpu' not in to_check:
        return False
    if to_check['max_cpu'] is None:
        return False

    if 'max_mem' not in to_check:
        return False
    if to_check['max_mem'] is None:
        return False

    if 'min_mem' not in to_check:
        return False
    if to_check['min_mem'] is None:
        return False

    return True


# Start

es = Elasticsearch(["sspl-perf-mon"])

perf_index = "perf-custom"
metrics_index = "metric*"

es.indices.refresh(index=perf_index)


# sspl-perform1 - the machine which is running our perf tests.
res = es.search(index=perf_index, body={"query": {"match": {"hostname": "sspl-perform1"}}}, size=1000)

task_names = []
prod_versions = []
tasks = []
days = []

# TODO update this filter based on slave script
task_filter = ["event1", "event2"]


# Build up lists of task names, days etc.
for hit in res['hits']['hits']:

    # Skip events / test loads we are not interested in.
    if hit["_source"]["eventname"] not in task_filter:
        continue

    # Build list of tasks
    if hit["_source"]["eventname"] not in task_names:
        task_names.append(hit["_source"]["eventname"])

    # Build list of product versions
    if hit["_source"]["product_version"] not in prod_versions:
        prod_versions.append(hit["_source"]["product_version"])

    t = create_task(hit["_source"])

    # Build list of days
    if t["day"] not in days:
        days.append(t["day"])

    if t is not None:
        print(t)
        tasks.append(t)


result_root = {}
for day in days:
    result_root[day] = {}
    for product_version in prod_versions:
        result_root[day][product_version] = {}
        for task_name in task_names:
            result_root[day][product_version][task_name] = []
            for task in tasks:
                if task['day'] == day and task['name'] == task_name and task['product_version'] == product_version:
                    result_root[day][product_version][task_name].append(task)


summary_root = {}
for day in result_root:
    summary_root[day] = {}

    for version in result_root[day]:
        summary_root[day][version] = {}

        for task_name in result_root[day][version]:
            summary_root[day][version][task_name] = {}
            good_result_count = 0
            summary_root[day][version][task_name]['avg_cpu'] = 0
            summary_root[day][version][task_name]['avg_mem'] = 0

            summary_root[day][version][task_name]['max_cpu'] = None
            summary_root[day][version][task_name]['max_mem'] = None
            summary_root[day][version][task_name]['min_mem'] = None

            for result in result_root[day][version][task_name]:

                if all_fields_present(result):
                    summary_root[day][version][task_name]['avg_cpu'] += result['avg_cpu']
                    summary_root[day][version][task_name]['avg_mem'] += result['avg_mem']

                    if summary_root[day][version][task_name]['max_cpu'] is None or result['max_cpu'] > summary_root[day][version][task_name]['max_cpu']:
                        summary_root[day][version][task_name]['max_cpu'] = result['max_cpu']

                    if summary_root[day][version][task_name]['max_mem'] is None or result['max_mem'] > summary_root[day][version][task_name]['max_mem']:
                        summary_root[day][version][task_name]['max_mem'] = result['max_mem']

                    if summary_root[day][version][task_name]['min_mem'] is None or result['min_mem'] < summary_root[day][version][task_name]['min_mem']:
                        summary_root[day][version][task_name]['min_mem'] = result['min_mem']

                    good_result_count += 1

            if good_result_count == 0:
                continue

            summary_root[day][version][task_name]['avg_cpu'] /= good_result_count
            summary_root[day][version][task_name]['avg_mem'] /= good_result_count

            summary_root[day][version][task_name]['avg_cpu'] *= 100
            summary_root[day][version][task_name]['avg_cpu'] = "{0:.2f}%".format(round(summary_root[day][version][task_name]['avg_cpu'], 2))

            summary_root[day][version][task_name]['max_cpu'] *= 100
            summary_root[day][version][task_name]['max_cpu'] = "{0:.2f}%".format(round(summary_root[day][version][task_name]['max_cpu'], 2))

            summary_root[day][version][task_name]['avg_mem'] /= 1000000
            summary_root[day][version][task_name]['avg_mem'] = "{0:.0f}MB".format(round(summary_root[day][version][task_name]['avg_mem'], 2))

            summary_root[day][version][task_name]['max_mem'] /= 1000000
            summary_root[day][version][task_name]['max_mem'] = "{0:.0f}MB".format(round(summary_root[day][version][task_name]['max_mem'], 2))

            summary_root[day][version][task_name]['min_mem'] /= 1000000
            summary_root[day][version][task_name]['min_mem'] = "{0:.0f}MB".format(round(summary_root[day][version][task_name]['min_mem'], 2))


print(json.dumps(summary_root))

with open('perf-results.json', 'w') as json_file:
    json_file.write(json.dumps(summary_root))
    print("wrote out perf-results.json")


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

    print(from_datetime)
    print(to_datetime)

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


# Start

es = Elasticsearch(["sspl-perf-mon"])

perf_index = "perf-custom"
metrics_index = "metric*"

es.indices.refresh(index=perf_index)

# sspl-perform1

res = es.search(index=perf_index, body={"query": {"match": {"hostname": "sspl-perform1"}}}, size=1000)

task_names = []
prod_versions = []
tasks = []
days = []
task_filter = ["event1", "event2"]


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

#########


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

            for result in result_root[day][version][task_name]:
                if 'avg_cpu' in result and result['avg_cpu'] is not None and 'avg_mem' in result and result['avg_mem'] is not None:
                    summary_root[day][version][task_name]['avg_cpu'] += result['avg_cpu']
                    summary_root[day][version][task_name]['avg_mem'] += result['avg_mem']

                    good_result_count += 1

            summary_root[day][version][task_name]['avg_cpu'] /= good_result_count
            summary_root[day][version][task_name]['avg_mem'] /= good_result_count

            summary_root[day][version][task_name]['avg_cpu'] = summary_root[day][version][task_name]['avg_cpu'] * 100
            summary_root[day][version][task_name]['avg_mem'] = summary_root[day][version][task_name]['avg_mem'] / 1000000


            # print("avg_cpu for:")
            # print(day)
            # print(version)
            # print(task_name)
            # print(summary_root[day][version][task_name]['avg_cpu'])
            #









print(summary_root)

exit()

                        ##### old way based on product version
# This is the root of our result set.
root = {}

# Put results into struct like:  root["my event"]["0.5.1"]['results']
for name in task_names:
    root[name] = {}
    for version in prod_versions:
        root[name][version] = {}

        for task in tasks:
            if name == task['name'] and version == task['product_version']:

                if 'results' not in root[name][version]:
                    root[name][version]['results'] = []
                root[name][version]['results'].append(task)

# Create summary of results like:  root["my event"]["0.5.1"]['summary']['agg_avg_cpu']
for taskname in root:
    for version in root[taskname]:
        print(root[taskname][version])
        root[taskname][version]['summary'] = {}
        root[taskname][version]['summary']['agg_avg_cpu'] = 0
        root[taskname][version]['summary']['agg_max_cpu'] = 0
        root[taskname][version]['summary']['agg_avg_mem'] = 0
        root[taskname][version]['summary']['agg_max_mem'] = 0
        root[taskname][version]['summary']['agg_min_mem'] = 0
        root[taskname][version]['summary']['agg_duration'] = 0

        number_of_results = 0
        for r in root[taskname][version]['results']:

            # skip bad runs with no data.
            if 'avg_cpu' in r and 'max_cpu' in r and 'avg_mem' in r and 'max_mem' in r and 'min_mem' in r and 'duration' in r:
                if r['avg_cpu'] is not None and r['avg_mem'] is not None and r['duration'] is not None and r['max_cpu'] is not None and r['max_mem'] is not None and r['min_mem'] is not None:
                    root[taskname][version]['summary']['agg_avg_cpu'] += r['avg_cpu']
                    root[taskname][version]['summary']['agg_avg_mem'] += r['avg_mem']
                    root[taskname][version]['summary']['agg_duration'] += r['duration']

                    if r['max_cpu'] > root[taskname][version]['summary']['agg_max_cpu']:
                        root[taskname][version]['summary']['agg_max_cpu'] = r['max_cpu']

                    if r['max_mem'] > root[taskname][version]['summary']['agg_max_mem']:
                        root[taskname][version]['summary']['agg_max_mem'] = r['max_mem']

                    if r['min_mem'] < root[taskname][version]['summary']['agg_min_mem']:
                        root[taskname][version]['summary']['agg_min_mem'] = r['min_mem']

                    number_of_results += 1

        if number_of_results == 0:
            continue
        root[taskname][version]['summary']['agg_avg_cpu'] /= number_of_results
        root[taskname][version]['summary']['agg_avg_mem'] /= number_of_results
        root[taskname][version]['summary']['agg_duration'] /= number_of_results

        # Throw away individual results now that we have a summary.
        del root[taskname][version]['results']


print("---")
print(json.dumps(root))
print("---")


with open('perf-results.json', 'w') as json_file:
    json_file.write(json.dumps(root))
    print("wrote out perf-results.json")


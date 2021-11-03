import os
import json

from datetime import datetime
def get_run_time_from_file(filepath):
    content = []
    with open("./results/"+filepath, 'r') as f:
        content = f.readlines()
    starttime = ""
    for line in content:
        if "starttime" in line:
            starttime = line
            break

    endtime = ""
    for line in reversed(content):
        if "endtime" in line:
            endtime = line
            break
    start = extract_timestamp("starttime",starttime)
    end = extract_timestamp("endtime",endtime)
    FMT = '%H:%M:%S.%f'
    tdelta = datetime.strptime(end, FMT) - datetime.strptime(start, FMT)


    return round(tdelta.seconds/60)


def extract_timestamp(key, xml):
    timestamp = xml.split(key)[1].split("\"")[1][9:]
    return timestamp

def main():
    files = os.listdir("results")
    outputfiles = []
    for file in files:
        if "output.xml" in file:
            outputfiles.append(file)
    data = {}
    for file in outputfiles:
        key = file[:-11]
        index = key.find("LOAD")
        loadtag = key[index-4:]
        time = get_run_time_from_file(file)
        try:
            count = data[loadtag]["count"]
            count = count + 1
            data[loadtag]["count"] = count
        except KeyError:
            data[loadtag] = {}
            data[loadtag]["count"] = 1


        data[loadtag][key] = time
    for loadtag in data:
        total = 0
        for machine in data[loadtag]:
            if machine != "count":
                total = total + data[loadtag][machine]
        data[loadtag]["average"] = round(total/data[loadtag]["count"], 1)
    string = json.dumps(data).split(",")

    with open("test.html", 'w') as f:
        for line in string:
            f.write("<pre>" + line + "</pre>\n")

main()
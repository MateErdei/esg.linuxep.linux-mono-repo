import datetime
import boto3
import sys
import pytz
import re

def main(argv):

    stackname = argv[1]
    date_regex = r"([0-9]{4})-(0[0-9]|1[0-2])-([0-2][0-9]|3[0-1])"
    date = re.search(date_regex, stackname)

    current_date = datetime.datetime(int(date.group(1)), int(date.group(2)), int(date.group(3)))
    limit = current_date - datetime.timedelta(days=30)

    print(limit)
    s3 = boto3.resource('s3')
    folders = s3.meta.client.list_objects(Bucket="sspl-testbucket", Prefix="test-results")

    keys_to_remove = []
    for folder in folders['Contents']:

        if folder['LastModified'] < pytz.UTC.localize(limit):
            if folder['Key'] != "test-results/":
                keys_to_remove.append(folder)

    delete_keys = {'Objects': [{'Key': k} for k in [obj['Key'] for obj in keys_to_remove]]}
    print(delete_keys)
    if len(delete_keys['Objects']) > 0:
        s3.meta.client.delete_objects(Bucket="sspl-testbucket", Delete=delete_keys)

if __name__ == '__main__':
    sys.exit(main(sys.argv))
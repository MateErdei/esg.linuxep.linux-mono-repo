import yaml
import sys

warehouse_to_check = sys.argv[1]

try:
    with open(warehouse_to_check, "r") as warehouse:
        yaml.dump(yaml.load(warehouse))

    print "You can checkin now..."
except Exception as ex:
    print ex
    print "Don't checkin - you'll break the build!"
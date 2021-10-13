import shutil
import sys

def main():
    total, used, free = shutil.disk_usage("/")
    percentage = free/total
    print("Total: %d GiB" % (total // (2**30)))
    print("Used: %d GiB" % (used // (2**30)))
    print("Free: %d GiB" % (free // (2**30)))
    if percentage < 0.2:
        print("Running out of space please take action, less than 10 percent space left")
        sys.exit(1)
    else:
        print("all good")


if __name__ == "__main__":
    sys.exit(main())

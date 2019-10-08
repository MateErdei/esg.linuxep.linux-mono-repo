import sys

import mcsrouter.register_central as register_central

if __name__ == "__main__":
    import coverage
    cov = coverage.Coverage(data_file="/tmp/register_central", data_suffix=True)
    cov.start()
    code = 1
    try:
        code = register_central.main(sys.argv)
    finally:
        cov.stop()
        cov.save()
    sys.exit(code)
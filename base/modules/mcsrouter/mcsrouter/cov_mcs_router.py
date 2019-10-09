import sys

import mcsrouter.mcs_router as mcs_router

if __name__ == "__main__":
    import coverage
    cov = coverage.Coverage(data_file="/tmp/mcs_router_coverage",
                            data_suffix=False, cover_pylib=False, auto_data=True, branch=True)
    cov.start()
    code = 1
    try:
        code = mcs_router.main()
    finally:
        cov.stop()
        cov.save()
    sys.exit(code)
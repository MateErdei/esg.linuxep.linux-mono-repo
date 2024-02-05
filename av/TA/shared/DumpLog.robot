
*** Settings ***
Library         ../Libs/LogUtils.py

*** Keywords ***

on fail dump logs
    dump log  ${WATCHDOG_LOG}
    dump log  ${SOPHOS_INSTALL}/logs/base/wdctl.log
    dump log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log
    dump log  ${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log
    dump log  ${TELEMETRY_LOG_PATH}
    dump log  ${AV_INSTALL_LOG}
    dump log  ${AV_LOG_PATH}
    dump log  ${SUSI_DEBUG_LOG_PATH}
    dump log  ${THREAT_DETECTOR_LOG_PATH}
    dump log  ${SAFESTORE_LOG_PATH}
    dump log  ${ON_ACCESS_LOG_PATH}

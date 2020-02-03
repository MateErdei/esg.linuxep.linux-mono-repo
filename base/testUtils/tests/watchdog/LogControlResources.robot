*** Settings ***

Library    OperatingSystem
Library    Collections
Library    ${libs_directory}/FullInstallerUtils.py
Library    ${libs_directory}/OSUtils.py

Resource  WatchdogResources.robot


*** Keywords ***

Get Log Content For Component
    [Arguments]  ${componentName}
    ${relativePath} =  get_relative_log_path_for_log_component_name  ${componentName}
    ${fullPath} =  Set Variable  ${SOPHOS_INSTALL}/${relativePath}
    ${logContent}  Get File  ${fullPath}
    [return]  ${logContent}

Get Log Content For Component And Clear It
    [Arguments]  ${componentName}
    ${relativePath} =  get_relative_log_path_for_log_component_name  ${componentName}
    ${fullPath} =  Set Variable  ${SOPHOS_INSTALL}/${relativePath}
    ${logContent}  Get File  ${fullPath}
    Remove File  ${fullPath}
    [return]  ${logContent}


Restart Plugin And Return Its Log File
    [Arguments]  ${pluginName}  ${componentName}
    #Plugin may not be installed yet so stop will return error, ignore
    Run Keyword and Ignore Error   Wdctl stop plugin  ${pluginName}

    #Plugin may not have logged anything yet, ignore
    ${previousLog} =  Run Keyword and Ignore Error  Get Log Content For Component And Clear It  ${componentName}

    #Plugin may not be installed yet so start will return error, ignore
    Run Keyword and Ignore Error  Wdctl Start Plugin  ${pluginName}

    [return]  ${previousLog}

Override LogConf File as Global Level
    [Arguments]  ${logLevel}  ${key}=VERBOSITY
    ${LOGGERCONF} =  get_logger_conf_path
    Create File  ${LOGGERCONF}  [global]\n${key} = ${logLevel}\n

Set Log Level For Component Plus Subcomponent And Reset and Return Previous Log
    [Arguments]  ${componentName}  ${logLevel}  ${keyValue}=VERBOSITY   &{kwargs}
    Log  Setting loglevel to ${logLevel} for component: ${componentName} and subcomponents &{kwargs}
    ${loggerConfContent} =  Get the LoggerConf file For   ${componentName}  ${keyValue}  ${logLevel}  &{kwargs}

    ${LOGGERCONF} =  Get Logger Conf Path
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  File Should Exist  ${LOGGERCONF}

    # use append to file to avoid keywords interfering with one another.
    # if two keywords try to set the loglevel of two different components ( a plugin and suldownloader, for example) both should be enabled
    # by using the append option
    Append To File If Not In File  ${LOGGERCONF}  ${loggerConfContent}

    ${loggerConfFile}=  Get File  ${LOGGERCONF}

    Log  ${loggerConfFile}

    ${pluginName} =  Get Plugin Name For Log Component Name   ${componentName}

    # it is not an error not being able to restart a plugin as this keyword may be called before the plugin is ever
    # started or with the plugin not part of watchdog.
    ${previousLogAndFlag} =  Restart Plugin And Return Its Log File  ${pluginName}  ${componentName}
    ${previousLog}  Get From List  ${previousLogAndFlag}  -1
    [return]  ${previousLog}

Set Log Level For Component And Reset and Return Previous Log
    [Arguments]  ${componentName}  ${logLevel}
    ${value} =  Set Log Level For Component Plus Subcomponent And Reset and Return Previous Log  ${componentName}  ${logLevel}
    [return]  ${value}

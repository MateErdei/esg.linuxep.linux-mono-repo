*** Variables ***
${RuntimeDetectionsPluginDir}  ${SOPHOS_INSTALL}/plugins/runtimedetections
${RuntimeDetectionsExecutableName}  capsule8-sensor
${RuntimeDetectionsExecutablePath}  ${RuntimeDetectionsPluginDir}/bin/${RuntimeDetectionsExecutableName}
*** Keywords ***
Check Runtime Detections Plugin Installed
    File Should Exist   ${RuntimeDetectionsExecutablePath}
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check Runtime Detections Plugin Running

Check Runtime Detections Plugin Running
    ${result} =    Run Process  pgrep  ${RuntimeDetectionsExecutableName}
    Should Be Equal As Integers    ${result.rc}    0

Check Runtime Detections Plugin Not Running
    ${result} =    Run Process  pgrep  ${RuntimeDetectionsExecutableName}
    Should Be Equal As Integers    ${result.rc}    1

Check Runtime Detections Plugin Not Installed
    Directory Should Not Exist  ${RuntimeDetectionsPluginDir}
    Check Runtime Detections Plugin Not Running

set (EXCLUDED_DIRS ${CMAKE_SOURCE_DIR}/tests/googletest ${CMAKE_SOURCE_DIR}/modules/pluginimpl/cmake-build-debug
        ${CMAKE_SOURCE_DIR}/CMakeFiles)

file(GLOB_RECURSE ALL_SOURCE_FILES *.cpp *.h *.hpp)

list (REMOVE_ITEM  ALL_SOURCE_FILES ${EXCLUDED_DIRS})

foreach (EXCLUDE ${EXCLUDED_DIRS})
    foreach (SOURCE_FILE ${ALL_SOURCE_FILES})
        string(FIND ${SOURCE_FILE} ${EXCLUDE} EXCLUDE_FOUND)

        if (NOT ${EXCLUDE_FOUND} EQUAL -1)
            list(REMOVE_ITEM ALL_SOURCE_FILES ${SOURCE_FILE})
        endif()
    endforeach ()
endforeach ()

add_custom_target(
        cppcheck
        COMMAND /usr/bin/cppcheck
        --enable=all
        --inline-suppr
        --std=c++17
        --language=c++
        --template="[{severity}][{id}] {message} {callstack} \(On {file}:{line}\)"
        --verbose
        -I ${pluginapiinclude}/Common
        -I ${pluginapiinclude}/log4cplus
        -I ${testhelpersinclude}
        -i ${CMAKE_SOURCE_DIR}/tests/googletest
        --quiet
        --force
        --xml
        ${CMAKE_SOURCE_DIR}
)
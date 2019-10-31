add_executable(LoggerLimit
        LoggerLimit.cpp
        )

add_executable(PickYourPoison
        PickYourPoison.cpp
        )

target_include_directories(LoggerLimit PUBLIC ${LOG4CPLUS_INCLUDE_DIR} )
target_link_libraries(LoggerLimit logging)

target_link_libraries(PickYourPoison pthread)
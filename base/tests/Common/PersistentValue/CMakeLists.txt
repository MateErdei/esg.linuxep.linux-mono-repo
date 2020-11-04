include(GoogleTest)
add_executable(TestPersistentValue
        ${CMAKE_SOURCE_DIR}/modules/Common/PersistentValue/PersistentValue.h
#        ${CMAKE_SOURCE_DIR}/modules/Common/PersistentValue/Logger.h
#        ${CMAKE_SOURCE_DIR}/modules/Common/PersistentValue/Logger.cpp
        TestPersistentValue.cpp)
target_include_directories(TestPersistentValue SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include" ${CMAKE_SOURCE_DIR})
target_include_directories(TestPersistentValue BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${gmock_SOURCE_DIR}" )
target_link_libraries(TestPersistentValue gtest gtest_main gmock threads filesystemimpl testhelpers)
gtest_discover_tests(TestPersistentValue)

#SET_TARGET_PROPERTIES( TestPersistentValue
#        PROPERTIES
#        INSTALL_RPATH "$ORIGIN"
#        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs")
#
#install(TARGETS TestPersistentValue
#        LIBRARY DESTINATION files/base/lib64)


#target_link_libraries(httpsenderimpl PUBLIC
#        -L"${CMAKE_BINARY_DIR}/libs"
#        ssl
#        crypto
#        ${CURL_LIBRARY}
#        ${LOG4CPLUS_LIBRARY}
#        logging
#        )
#
#
#
#install(TARGETS httpsenderimpl
#        LIBRARY DESTINATION files/base/lib64)

#add_dependencies(httpsenderimpl copy_libs)
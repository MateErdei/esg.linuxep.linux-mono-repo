#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "GTest::gtest" for configuration "Debug"
set_property(TARGET GTest::gtest APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(GTest::gtest PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LINK_INTERFACE_LIBRARIES_DEBUG "Threads::Threads"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libgtestd.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS GTest::gtest )
list(APPEND _IMPORT_CHECK_FILES_FOR_GTest::gtest "${_IMPORT_PREFIX}/lib/libgtestd.a" )

# Import target "GTest::gtest_main" for configuration "Debug"
set_property(TARGET GTest::gtest_main APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(GTest::gtest_main PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LINK_INTERFACE_LIBRARIES_DEBUG "Threads::Threads;GTest::gtest"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libgtest_maind.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS GTest::gtest_main )
list(APPEND _IMPORT_CHECK_FILES_FOR_GTest::gtest_main "${_IMPORT_PREFIX}/lib/libgtest_maind.a" )

# Import target "GTest::gmock" for configuration "Debug"
set_property(TARGET GTest::gmock APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(GTest::gmock PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LINK_INTERFACE_LIBRARIES_DEBUG "Threads::Threads;GTest::gtest"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libgmockd.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS GTest::gmock )
list(APPEND _IMPORT_CHECK_FILES_FOR_GTest::gmock "${_IMPORT_PREFIX}/lib/libgmockd.a" )

# Import target "GTest::gmock_main" for configuration "Debug"
set_property(TARGET GTest::gmock_main APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(GTest::gmock_main PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LINK_INTERFACE_LIBRARIES_DEBUG "Threads::Threads;GTest::gmock"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libgmock_maind.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS GTest::gmock_main )
list(APPEND _IMPORT_CHECK_FILES_FOR_GTest::gmock_main "${_IMPORT_PREFIX}/lib/libgmock_maind.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)

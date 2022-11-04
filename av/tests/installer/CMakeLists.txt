
SophosAddTest(TestSyncVersionedFiles
        TestSyncVersionedFiles.cpp
        ${CMAKE_SOURCE_DIR}/products/installer/sync_versioned_files/sync_versioned_files.cpp
        ${CMAKE_SOURCE_DIR}/products/installer/sync_versioned_files/sync_versioned_files.h
        LIBS ${STD_FILESYSTEM_IF_REQUIRED}
             ${pluginapilib}
             datatypes
        INC_DIRS ${CMAKE_SOURCE_DIR}/products/installer/sync_versioned_files
                 ${pluginapiinclude}
        )

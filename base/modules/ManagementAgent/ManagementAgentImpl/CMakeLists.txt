add_library(managementagentimpl SHARED
        Logger.h
        ManagementAgentMain.h
        ManagementAgentMain.cpp)
target_include_directories(managementagentimpl PUBLIC  ../../Common )
target_link_libraries(managementagentimpl PUBLIC pluginapiimpl applicationconfigurationimpl plugincommunicationimpl)
install(TARGETS managementagentimpl
        LIBRARY DESTINATION files/base/lib64)
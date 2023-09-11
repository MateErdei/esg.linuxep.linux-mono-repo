add_library(machineidlib STATIC
        MachineId.cpp
        MachineId.h)

target_link_libraries(machineidlib PUBLIC filesystemimpl osutilitiesimpl applicationconfigurationimpl)

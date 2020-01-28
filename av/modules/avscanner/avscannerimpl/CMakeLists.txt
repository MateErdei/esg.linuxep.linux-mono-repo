
add_library(avscannerimpl SHARED
        Options.cpp
        Options.h
        )

install(TARGETS avscannerimpl
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

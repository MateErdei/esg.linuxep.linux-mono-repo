
sophos_add_library(filewalker SHARED
        FileWalker.cpp
        FileWalker.h
        sophos_filesystem.h
        EXTRA_LIBS $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>>:stdc++fs>
        )

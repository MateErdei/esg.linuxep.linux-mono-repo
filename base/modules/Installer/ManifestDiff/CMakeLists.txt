add_library(manifestdifflib STATIC
        ManifestDiff.cpp
        ManifestDiff.h
        CommandLineOptions.cpp
        CommandLineOptions.h
        Manifest.cpp
        Manifest.h
        ManifestEntry.cpp
        ManifestEntry.h
        ../../Common/UtilityImpl/StringUtils.h
        ../../Common/UtilityImpl/iostr_utils.h
        )

target_link_libraries(manifestdifflib PUBLIC filesystemimpl)

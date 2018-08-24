/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "SulDownloader.h"

#include <Common/UtilityImpl/Main.h>

static int sul_downloader_main(int argc, char * argv[])
{
    return SulDownloader::main_entry(argc, argv);
}

MAIN(sul_downloader_main(argc,argv))

// Copyright 2018-2023 Sophos Limited. All rights reserved.
#include "Common/Main/Main.h"

#include "SulDownloader/SulDownloader.h"

static int sul_downloader_main(int argc, char* argv[])
{
    return SulDownloader::main_entry(argc, argv);
}

MAIN(sul_downloader_main(argc, argv))

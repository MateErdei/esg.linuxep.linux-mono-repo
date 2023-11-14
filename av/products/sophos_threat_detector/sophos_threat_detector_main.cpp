// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "sophos_threat_detector_inner_main.h"

#include "Common/Main/Main.h"

static int outer_main()
{
    return inner_main();
}
MAIN(outer_main())

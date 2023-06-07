// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

// define a macro which helps ensure that exceptions always contain useful
// information.  Use it as the first argument to the exception constructor to
// ensure that the correct location information is included.
// Usage:
//
//        throw GeneralException(LOCATION, "DB007", "This martini has been stirred!");
//

#define LOCATION __FILE__,__LINE__

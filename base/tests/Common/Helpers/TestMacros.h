#pragma once
#include <iostream>

bool skipTest();
bool skipIfCoverage();

#define MAYSKIP if(skipTest()) GTEST_SKIP() << "must be root to run this test"

#define SKIPIFCOVERAGE  if (skipIfCoverage()) GTEST_SKIP() << "can't run this test during coverage"


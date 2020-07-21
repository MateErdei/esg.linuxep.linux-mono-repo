#pragma once
#include <iostream>

bool skipTest();
bool skipIfCoverage();

#define MAYSKIP if(skipTest()) return

#define SKIPIFCOVERAGE  if (skipIfCoverage()) return


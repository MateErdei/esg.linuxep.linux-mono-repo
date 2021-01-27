
// Copyright 2011 Sophos Limited. All rights reserved.
#pragma once

#define CRTDBG_MAP_ALLOC

#include <gtest/gtest.h>

#include <stdlib.h>
//#include <crtdbg.h>

// -----------------------------------------------------------------------------------------------
// Memory leak detection for use with Google Test
// -----------------------------------------------------------------------------------------------

template<typename T>
class MemoryLeakDetectionFixtureTemplate : public T
{
public:

    MemoryLeakDetectionFixtureTemplate() : iFlags_(0)
    {

#ifdef _DEBUG
        iFlags_ = _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF);

		static_cast<void>(_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE));
		static_cast<void>(_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT));

		_CrtMemCheckpoint(&memBefore_);

#endif // _DEBUG

    }

    virtual ~MemoryLeakDetectionFixtureTemplate()
    {

#ifdef _DEBUG
        if (!T::HasFailure())
		{
			_CrtMemState memAfter;
			_CrtMemCheckpoint(&memAfter);

			//	Dump memory if a leak is detected. Check the count field of memDifference to see the count
			//	of leaked blocks, and what sort of blocks they are. The count field has five elements - one
			//	for each of the following block types:
			//
			//	0 - Free (should always be zero, because we do not track free blocks)
			//	1 - Normal (this is the type of block normally leaked)
			//	2 - CRT (blocks allocated by the runtime library)
			//	3 - Ignore (should always be zero)
			//	4 - Client (normally zero, although MFC can allocate this type of block)

			_CrtMemState memDifference;

			if (_CrtMemDifference(&memDifference, &memBefore_, &memAfter))
			{
				static_cast<void>(_CrtMemDumpAllObjectsSince(&memBefore_));

				GTEST_NONFATAL_FAILURE_("Memory leak detected.");
			}
		}

		static_cast<void>(_CrtSetDbgFlag(iFlags_));

#endif // _DEBUG

    }

//#pragma warning(push)
//#pragma warning(disable : 4100)					// Unreferenced formal parameter

    void BreakAtAllocationNumber(long lAllocation)
    {
        //static_cast<void>(_CrtSetBreakAlloc(lAllocation));
    }

//#pragma warning(pop)

private:

    int iFlags_{};

    //_CrtMemState memBefore_{};
};

typedef MemoryLeakDetectionFixtureTemplate<::testing::Test> MemoryLeakDetectionFixture;



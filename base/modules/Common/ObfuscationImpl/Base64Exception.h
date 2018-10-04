/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <stdexcept>

// A simple exception class for the crypto class.
namespace Common
{
	namespace ObfuscationImpl
	{
		class Base64Exception
				: public std::runtime_error
		{
		public:
			using std::runtime_error::runtime_error;
		};
	}
}


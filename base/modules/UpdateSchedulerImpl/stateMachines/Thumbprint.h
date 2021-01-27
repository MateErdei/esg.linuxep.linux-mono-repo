/* Copyright 2012 Sophos Limited. All rights reserved.
 * Sophos and Sophos Anti-Virus are registered trademarks of Sophos Limited.
 * All other product and company names mentioned are trademarks or registered
 * trademarks of their respective owners.
 */

#pragma once

#include <string>

class Thumbprint
{
public:
	explicit Thumbprint(const std::string& thumbprint);

	std::string GetThumbprint() const;

	bool operator==(const Thumbprint& thumbprint) const;

	bool operator!=(const Thumbprint& thumbprint) const;

private:
	std::string m_thumbprint;
};

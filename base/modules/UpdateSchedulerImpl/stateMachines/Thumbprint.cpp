#include "Thumbprint.h"

Thumbprint::Thumbprint(const std::string& thumbprint) :
m_thumbprint(thumbprint)
{
}

std::string Thumbprint::GetThumbprint() const
{
	return m_thumbprint;
}

bool Thumbprint::operator==(const Thumbprint& thumbprint) const
{
	return GetThumbprint() == thumbprint.GetThumbprint();
}

bool Thumbprint::operator!=(const Thumbprint& thumbprint) const
{
	return GetThumbprint() != thumbprint.GetThumbprint();
}

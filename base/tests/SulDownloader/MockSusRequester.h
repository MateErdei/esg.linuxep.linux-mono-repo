// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "SulDownloader/sdds3/ISusRequester.h"

#include <gmock/gmock.h>

class MockSusRequester : public SulDownloader::SDDS3::ISusRequester
{
public:
    MOCK_METHOD(SulDownloader::SDDS3::SusResponse, request, (const SulDownloader::SUSRequestParameters& parameters));
};

#include <curl/curl.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <Common/HttpRequestsImpl/HttpRequesterImpl.h>
#include "../Helpers/LogInitializedTests.h"
#include <Common/CurlWrapper/CurlWrapper.h>

class HttpRequesterImplTests : public LogInitializedTests
{};

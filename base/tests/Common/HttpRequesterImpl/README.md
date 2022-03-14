#HttpRequester Tests


The tests for the HTTP lib are split into unit tests and live network google/TAP tests. 
We use the unit tests to check the business logic of the library code and the TAP tests to perform live network tests 
to ensure that the library really does actually work. The live tests are executed against HttpTestServer.py.

---

##HttpRequesterTests.cpp

Contains unit tests with a mocked out cURL


##HttpRequesterLiveNetworkTests.cpp

Live network tests that are run in TAP against a test server HttpTestServer.py

To run the live network tests locally run "python3 HttpTestServer.py" and then run the google tests in HttpRequesterLiveNetworkTests.cpp as normal, either via IDE or cmdline.


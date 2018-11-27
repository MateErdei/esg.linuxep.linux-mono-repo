CAPNPROTO files

capnp files are the definition files for the event messages we want to send.

The definition files get compiled using the capnp tool into C++ classes which can then be used within our code
to serialize and deserialize the binary string data to and from SSPL event classes.

The definition files can be found in the following perforce location.  Which is part of the windows server implementation.

//wininf/latest-dev/sed/src/capnproto/

Other files have been created from the specification provided on the wiki.
https://wiki.sophos.net/display/SL/Project%3A+Windows+Darwin+Sensors#Project:WindowsDarwinSensors-CommonDefinitions

The definition files are compiled when the EventTypesImpl code is complied via the CMAKE file by calling CAPN_GENERATE_CPP
function. At the moment all the definition files are compiled, but only the output files required for SSPL events are copied and used by the build.


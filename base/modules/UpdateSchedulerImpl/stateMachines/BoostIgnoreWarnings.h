// Disable Boost's headers from polluting our clean warnings.

#ifdef _WIN32
#include <CodeAnalysis/Warnings.h>
#pragma warning(disable: ALL_CODE_ANALYSIS_WARNINGS)
#pragma warning(disable: 6246) // Local declaration of 'xxx' hides declaration of the same name in outer scope.
#pragma warning(disable: 26135) // Missing locking annotation
//#pragma warning (disable: 4244)
//#pragma warning(disable : 4512)
//#pragma warning(disable : 4512)
//#pragma warning(disable : 4100 6011)
#endif

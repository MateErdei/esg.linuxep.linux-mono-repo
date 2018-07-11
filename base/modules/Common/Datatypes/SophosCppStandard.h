///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#ifndef EVEREST_BASE_SOPHOSCPPSTANDARD_H
#define EVEREST_BASE_SOPHOSCPPSTANDARD_H

#ifndef CPPSTD
# if __cplusplus <= 199711L
#  define CPPSTD 03
# else /* __cplusplus */
#  define CPPSTD 11
# endif /* __cplusplus */
#endif /* CPPSTD */

#if 03 == CPPSTD
#define STDMOVE(x) (x)
#define NOEXCEPT throw()
#define STRARG const std::string&
#define NULLPTR 0
#else
#define STDMOVE(x) std::move(x)
#define NOEXCEPT noexcept
#define STRARG std::string
#define NULLPTR nullptr
#endif

#endif //EVEREST_BASE_SOPHOSCPPSTANDARD_H

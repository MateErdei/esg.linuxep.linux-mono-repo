///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#ifndef VERSIG_SOPHOSCPPSTANDARD_H
#define VERSIG_SOPHOSCPPSTANDARD_H

#ifndef CPPSTD
# if __cplusplus <= 199711L
#  define CPPSTD 03
# else /* __cplusplus */
#  define CPPSTD 11
# endif /* __cplusplus */
#endif /* CPPSTD */

#endif //VERSIG_SOPHOSCPPSTANDARD_H

///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#ifndef VERSIG_VERSIG_H
#define VERSIG_VERSIG_H

#include <vector>
#include <string>

int versig_main(const std::vector<std::string>& argv);

int versig_main(
    int argc,		//[i] Count of arguments
    char* argv[]	//[i] Array of argument values
);

#endif //VERSIG_VERSIG_H

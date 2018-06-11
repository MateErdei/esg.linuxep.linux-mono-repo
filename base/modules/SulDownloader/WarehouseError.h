//
// Created by pair on 11/06/18.
//

#ifndef EVEREST_BASE_WAREHOUSEERROR_H
#define EVEREST_BASE_WAREHOUSEERROR_H

#include <string>
namespace SulDownloader
{
    enum WarehouseStatus{ SUCCESS=0, INSTALLFAILED=-1, DOWNLOADFAILED=-2, RESTARTNEEDED=-3, CONNECTIONERROR=-4, PACKAGESOURCEMISSING=-5, UNSPECIFIED=-6 };
    std::string toString( WarehouseStatus );
    struct WarehouseError
    {
        std::string Description;
        std::string SulError;
        WarehouseStatus  status;
    };

}



#endif //EVEREST_BASE_WAREHOUSEERROR_H

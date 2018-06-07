//
// Created by pair on 06/06/18.
//

#ifndef EVEREST_SULUTILS_H
#define EVEREST_SULUTILS_H
extern "C" {
#include <SUL.h>
}
#include <string>
#include <vector>



namespace SulDownloader
{

    std::string SulGetErrorDetails( SU_Handle session );
    std::string SulGetLogEntry( SU_Handle session);
    std::string SulQueryProductMetadata( SU_PHandle product, const std::string & attribute, SU_Int index);
    std::string SulQueryDistributionFileData( SU_Handle session, SU_Int index, const std::string & attribute );

    struct Error{
        constexpr static int NoSulError = 90;
        Error(){}
        void fetchSulError( SU_Handle session);

        std::string Description;
        std::string SulError;
        int SulCode=0;
    };

    class SULUtils
    {
    public:
        static bool isSuccess(SU_Result result);
        static void displayLogs(SU_Handle ses);
        static std::vector<std::string> SulLogs(SU_Handle ses);



    };
}

#endif //EVEREST_SULUTILS_H

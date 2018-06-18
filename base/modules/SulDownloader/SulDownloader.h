/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_SULDOWNLOADER_H
#define EVEREST_SULDOWNLOADER_H
#include <tuple>
#include <string>
namespace SulDownloader
{
    class DownloadReport;
    class ConfigurationData;
    /**
     * Executes the core functionality of SULDownloader.
     *
     *  - Fetch a remote ::WarehouseRepository.
     *  - Synchronize it by selecting the products.
     *  - Distribute the products to local paths.
     *  - Verify the products are correctly signed.
     *  - Install the products
     *
     *  Produces the ::DownloadReport which contains the necessary information to keep track of the WarehouseRepository and Managed products.
     *
     *
     * @param configurationData which contains the full settings to SULDownloader to execute its jobs.
     * @return DownloadReport which in case of failure will conatain description of the problem,
     *         and usually also contain the list of products installed with relevant information for each product.
     * @pre Require that configurationData is already verified configurationData::verifySettingsAreValid
     * @note This method is not supposed to throw, as any failure is to be described in DownloadReport.
     */
    DownloadReport runSULDownloader( ConfigurationData & configurationData);


    /**
     * Run ::runSULDownloader whilst handling serialization of ::DownloadReport and ::ConfigurationData.
     *
     * @param settingsString serialized (json) version of SulDownloaderProto::ConfigurationSettings.
     * @return Pair containing the exit code and the serialized (json) version of  SulDownloaderProto::DownloadStatusReport
     *         The exit code follows the convention of 0 for success, otherwise failure
     * @pre settingsString is a valid serialized version of SulDownloaderProto::ConfigurationSettings.
     * @note If either the json parser fails to de-serialize settingsString or the ConfigurationData produced does not pass the ::verifySettingsAreValid
     *       it will not runSULDownloader and return the failure directly.
     */
    std::tuple<int, std::string> configAndRunDownloader( const std::string & settingsString);


    /**
     * Run configAndRunDownloader whilst handling files for input and output.
     *
     * It reads the inputFilePath and give its content as settingsString to configAndRunDownloader.
     * It writes the output of configAndRunDownloader into the outputFilePath and return the exit code.
     *
     * @param inputFilePath Json file of the input settings.
     * @param outputFilePath File where the report of SulDownloader will be written to.
     * @return The exit code.
     * @throws If it cannot read or write the files safely it will throw exception.
     */
    int fileEntriesAndRunDownloader( const std::string & inputFilePath, const std::string & outputFilePath );


    /**
     * To be used when parsing arguments from argv as received in int main( int argc, char * argv[]).
     *
     * It runs fileEntriesAndRunDownloader and forward its return.
     *
     * It accepts only the following usage:
     * argv => SulDownloader <InputPath> <OutputPath>
     * Hence, it require argc == 3 and passes the InputPath and OutputPath to fileEntriesAndRunDownloader.
     *
     * @param argc As convention, the number of valid entries in argv with the program as the first argument.
     * @param argv As convention the strings of arguments.
     * @return
     */
    int main_entry(int argc, char *argv[]);
}
#endif //EVEREST_SULDOWNLOADER_H

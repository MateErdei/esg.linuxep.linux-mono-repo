/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginUtils.h"

#include "Logger.h"

#include <cstring>
#include <lzma.h>
namespace Plugin
{
    bool PluginUtils::init_encoder(lzma_stream *strm, uint32_t preset)
    {
        // Initialize the encoder using a preset.
        lzma_ret ret = lzma_easy_encoder(strm, preset, LZMA_CHECK_CRC64);

        // Return successfully if the initialization went fine.
        if (ret == LZMA_OK)
            return true;

        // Something went wrong.
        const char *msg;
        switch (ret) {
            case LZMA_MEM_ERROR:
                msg = "Memory allocation failed";
                break;

            case LZMA_OPTIONS_ERROR:
                msg = "Specified preset is not supported";
                break;

            case LZMA_UNSUPPORTED_CHECK:
                msg = "Specified integrity check is not supported";
                break;

            default:
                msg = "Unknown error, possibly a bug";
                break;
        }

        LOGERROR( "Error initializing the encoder: " << msg << " with error code: " << ret);
        return false;
    }


    bool PluginUtils::compress(lzma_stream *strm, FILE *infile, FILE *outfile)
    {
        // This will be LZMA_RUN until the end of the input file is reached.
        // This tells lzma_code() when there will be no more input.
        lzma_action action = LZMA_RUN;

        // Buffers to temporarily hold uncompressed input
        // and compressed output.
        uint8_t inbuf[BUFSIZ];
        uint8_t outbuf[BUFSIZ];

        strm->next_in = NULL;
        strm->avail_in = 0;
        strm->next_out = outbuf;
        strm->avail_out = sizeof(outbuf);

        // Loop until the file has been successfully compressed or until
        // an error occurs.
        while (true) {
            // Fill the input buffer if it is empty.
            if (strm->avail_in == 0 && !feof(infile)) {
                strm->next_in = inbuf;
                strm->avail_in = fread(inbuf, 1, sizeof(inbuf),
                                       infile);

                if (ferror(infile)) {
                    LOGERROR("Read error: " << strerror(errno));
                    return false;
                }


                if (feof(infile))
                    action = LZMA_FINISH;
            }

            lzma_ret ret = lzma_code(strm, action);


            if (strm->avail_out == 0 || ret == LZMA_STREAM_END) {

                size_t write_size = sizeof(outbuf) - strm->avail_out;

                if (fwrite(outbuf, 1, write_size, outfile)
                    != write_size) {
                    LOGERROR("Write error:"<< strerror(errno));
                    return false;
                }

                // Reset next_out and avail_out.
                strm->next_out = outbuf;
                strm->avail_out = sizeof(outbuf);
            }

            // Normally the return value of lzma_code() will be LZMA_OK
            // until everything has been encoded.
            if (ret != LZMA_OK) {

                if (ret == LZMA_STREAM_END)
                    return true;

                // It's not LZMA_OK nor LZMA_STREAM_END,
                // so it must be an error code.
                const char *msg;
                switch (ret) {
                    case LZMA_MEM_ERROR:
                        msg = "Memory allocation failed";
                        break;
                    case LZMA_DATA_ERROR:
                        msg = "File size limits exceeded";
                        break;

                    default:
                        msg = "Unknown error, possibly a bug";
                        break;
                }

                LOGERROR("Encoder error: " << msg << " with error code: " << ret);;
                return false;
            }
        }
    }

    void PluginUtils::compressFile(const std::string filepath)
    {
        lzma_stream strm = LZMA_STREAM_INIT;

        // Initialize the encoder. If it succeeds, compress from
        // stdin to stdout. the shoudl be file handles fro the file to compressed and the output file
        FILE *myfile = fopen (filepath.c_str() , "r");
        FILE *output = fopen ("/tmp/myfile.xz" , "rw");

        bool success = init_encoder(&strm, 7);
        if (success)
            success = compress(&strm, myfile, output);

        // Free the memory allocated for the encoder. If we were encoding

        lzma_end(&strm);

        // Close stdout to catch possible write errors that can occur
        // when pending data is flushed from the stdio buffers.
        if (fclose(stdout)) {
            LOGERROR( "Write error: " << strerror(errno));

        }
    }

}
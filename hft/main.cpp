/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System ® ≣≡=-              **
**                                                                    **
**          Copyright © 2017 - 2022 by LLG Ryszard Gradowski          **
**                       All Rights Reserved.                         **
**                                                                    **
**  CAUTION! This application is an intellectual propery              **
**           of LLG Ryszard Gradowski. This application as            **
**           well as any part of source code cannot be used,          **
**           modified and distributed by third party person           **
**           without prior written permission issued by               **
**           intellectual property owner.                             **
**                                                                    **
\**********************************************************************/

#include <iostream>
#include <boost/json/src.hpp>
#include <easylogging++.h>

#include <exception>

#include "hft-config.h"

static std::string hft_version(void)
{
    return std::string(HFT_VERSION_MAJOR) + std::string(".") + std::string(HFT_VERSION_MINOR);
}

INITIALIZE_EASYLOGGINGPP

typedef int (*program)(int, char **);

//
// HFT tools entry points.
//

int draft_main(int argc, char *argv[]);
int hft_server_main(int argc, char *argv[]);
int hft_dukasemu_main(int argc, char *argv[]);

static struct
{
    const char *tool_name;
    program start_program;

} hft_programs[] = {
    { .tool_name = "draft",                    .start_program = &draft_main },
    { .tool_name = "server",                   .start_program = &hft_server_main },
    { .tool_name = "dukascopy-emulator",       .start_program = &hft_dukasemu_main }
};

int main(int argc, char *argv[])
{
    try
    {
        std::cout << "High Frequency Trading System - Professional Expert Advisor, version " << hft_version() << "\n";
        std::cout << " Copyright © 2017 - 2022 by LLG Ryszard Gradowski, All Rights Reserved.\n\n";

        if (argc == 1 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)
        {
            std::cout << "Usage:\n"
                      << "  hft <tool> [tool options]\n\n"
                      << "Available tools:\n"
                      << "  dukascopy-emulator        HFT TCP Client emulates Dukascopy forex trading\n"
                      << "                            platform using dukascopy historical CSV data\n\n"
                      << "  server                    HFT Trading TCP Server. Expert Advisor for\n"
                      << "                            production and testing purposes\n\n";

            return 0;
        }

//        prog_opts::variables_map vm;
//        prog_opts::store(prog_opts::command_line_parser(argc, argv).options(desc).allow_unregistered().run(), vm);
//        prog_opts::notify(vm);

        for (auto &p : hft_programs)
        {
            if (strcmp(p.tool_name, argv[1]) == 0)
            {
                return p.start_program(argc - 1, argv + 1);
            }
        }

        std::cerr << "Invalid tool. Type `hft --help´ for more informations\n\n";

        return 1;
    }
    catch (const std::exception &e)
    {
        std::cerr << "fatal error: " << e.what() << "\n";
    }
    catch (...)
    {
        std::cerr << "fatal error: General exception\n";
    }

    return 1;
}

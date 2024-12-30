/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System ® ≣≡=-              **
**                                                                    **
**          Copyright © 2017 - 2025 by LLG Ryszard Gradowski          **
**                       All Rights Reserved.                         **
**                                                                    **
**  CAUTION! This application is an intellectual property             **
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

#include "hft2ctrader-config.h"

static std::string hft2ctrader_version(void)
{
    return std::string(HFT2CTRADER_VERSION_MAJOR) + std::string(".") + std::string(HFT2CTRADER_VERSION_MINOR);
}

INITIALIZE_EASYLOGGINGPP

typedef int (*program)(int, char **);

//
// HFT2CTRADER entry points.
//

int draft_main(int argc, char *argv[]);
int hft2ctrade_bridge_main(int argc, char *argv[]);
int historical_data_feed_main(int argc, char *argv[]);

static struct
{
    const char *name;
    program start_program;

} hft2ctrader_programs[] = {
    { .name = "draft",                .start_program = &draft_main },
    { .name = "historical-data-feed", .start_program = &historical_data_feed_main },
    { .name = "bridge",               .start_program = &hft2ctrade_bridge_main }
};

int main(int argc, char *argv[])
{
    try
    {
        std::cout << "hft2ctrader - HFT ⇌  cTrader intermediary client, version " << hft2ctrader_version() << "\n";
        std::cout << "Copyright © 2017 - 2025 by LLG Ryszard Gradowski, All Rights Reserved.\n\n";

        if (argc == 1 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)
        {
            std::cout << "Usage:\n"
                      << "  hft2ctrader <command> [tool options]\n\n"
                      << "Available commands:\n"
                      << "  bridge                 Bridge between HFT and cTrader proxy\n"
                      << "  historical-data-feed   Download historical ticks to CSV file for particular instrument\n"
                      << "                         and specified time\n"
                      << "\n\nType ‘hft2ctrader <command> --help’ for more info about options for particular command.\n\n";

            return 0;
        }

        for (auto &p : hft2ctrader_programs)
        {
            if (strcmp(p.name, argv[1]) == 0)
            {
                return p.start_program(argc - 1, argv + 1);
            }
        }

        std::cerr << "Invalid command. Type ‘hft --help’ for more informations\n\n";

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

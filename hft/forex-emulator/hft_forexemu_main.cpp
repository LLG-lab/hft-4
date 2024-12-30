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

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>

#include <boost/program_options.hpp>

#include <hft_display_filter.hpp>

#include <easylogging++.h>

namespace prog_opts = boost::program_options;

static struct dukas_emulator_options_type
{
    std::string host;
    std::string port;
    std::vector<std::string> instruments;
    std::string sessid;
    std::string config_file_name;
    int bankroll;
    bool check_bankruptcy;
    bool invert_hft_decision;

} dukas_emulator_options;

#define hftOption(__X__) \
    dukas_emulator_options.__X__

#define hft_log(__X__) \
    CLOG(__X__, "forex_emulator")

static std::map<std::string, std::string> mk_instrument_info_map(const std::vector<std::string> &instruments)
{
    std::map<std::string, std::string> result;

    for (auto &x : instruments)
    {
        size_t delimiter_index = x.find_first_of(':');

        if (delimiter_index == std::string::npos)
        {
            std::string err_msg = "Required line <ticker>:<file_name>, no delimiter ‘:’ found in line: ‘"
                                  + x + "’";

            throw std::runtime_error(err_msg.c_str());
        }

        std::string ticker = x.substr(0, delimiter_index);

        if (ticker.length() == 0)
        {
            std::string err_msg = "No ticker specified in line ‘" + x + "’";

            throw std::runtime_error(err_msg.c_str());
        }

        std::string file_name = x.substr(delimiter_index+1, x.length()-delimiter_index);

        if (file_name.length() == 0)
        {
            std::string err_msg = "No file name specified in line ‘" + x + "’";

            throw std::runtime_error(err_msg.c_str());
        }

        if (result.find(ticker) != result.end())
        {
            std::string err_msg = "Duplicate ticker ‘" + ticker + "’";

            throw std::runtime_error(err_msg.c_str());
        }

        result[ticker] = file_name;
    }

    return result;
}

int hft_dukasemu_main(int argc, char *argv[])
{
    //
    // Define default logger configuration.
    //

    el::Configurations logger_cfg;
    logger_cfg.setToDefault();
    logger_cfg.parseFromText("* GLOBAL:\n"
                             " FORMAT               =  \"%datetime %level [%logger] %msg\"\n"
                             " FILENAME             =  \"/dev/null\"\n"
                             " ENABLED              =  true\n"
                             " TO_FILE              =  false\n"
                             " TO_STANDARD_OUTPUT   =  true\n"
                             " SUBSECOND_PRECISION  =  1\n"
                             " PERFORMANCE_TRACKING =  true\n"
                             " MAX_LOG_FILE_SIZE    =  10485760 ## 10MiB\n"
                             " LOG_FLUSH_THRESHOLD  =  1 ## Flush after every single log\n"
                            );

    el::Loggers::setDefaultConfigurations(logger_cfg);

    START_EASYLOGGINGPP(argc, argv);

    //
    // Parsing options for emulator.
    //

    prog_opts::options_description hidden("Hidden options");
    hidden.add_options()
        ("forex-emulator", "")
    ;

    prog_opts::options_description desc("Options for forex-emulator");
    desc.add_options()
        ("help,h", "produce help message")
        ("host,H", prog_opts::value<std::string>(&hftOption(host)) -> default_value("localhost"), "HFT server hostname or ip address.")
        ("port,P", prog_opts::value<std::string>(&hftOption(port)) -> default_value("8137"), "HFT server listen port.")
        ("instrument,i", prog_opts::value<std::vector<std::string>>(&hftOption(instruments)), "<ticker>:<csv_file_name_or_file_name_with_list_of_csv_files>")
        ("sessid,s", prog_opts::value<std::string>(&hftOption(sessid)) -> default_value("forex-emulator"), "Session ID")
        ("bankroll,b", prog_opts::value<int>(&hftOption(bankroll)) -> default_value(10000), "Initial virtual deposit")
        ("check-bankruptcy,B", prog_opts::value<bool>(&hftOption(check_bankruptcy)) -> default_value(false), "Stop simulation when equity drops to zero")
        ("invert-hft-decision,I", prog_opts::value<bool>(&hftOption(invert_hft_decision)) -> default_value(false), "Play the opposite of the HFT decision")
        ("config,c", prog_opts::value<std::string>(&hftOption(config_file_name)) -> default_value("/etc/hft/hft-config.xml"), "HFT configuration file name")
    ;

    prog_opts::options_description cmdline_options;
    cmdline_options.add(desc).add(hidden);

    prog_opts::variables_map vm;
    prog_opts::store(prog_opts::command_line_parser(argc, argv).options(cmdline_options).run(), vm);
    prog_opts::notify(vm);

    //
    // If user requested help, show help and quit
    // ignoring other options, if any.
    //

    if (vm.count("help"))
    {
        std::cout << desc << "\n";

        return 0;
    }

    //
    // Setup logging.
    //

    el::Logger *logger = el::Loggers::getLogger("forex_emulator", true);

    if (hftOption(instruments).size() == 0)
    {
        hft_log(ERROR) << "No instrument specified.";

        return 1;
    }

    try
    {

        hft_forex_emulator simulation(hftOption(host),
                                      hftOption(port),
                                      hftOption(sessid),
                                      mk_instrument_info_map(hftOption(instruments)),
                                      hftOption(bankroll),
                                      hftOption(config_file_name),
                                      hftOption(check_bankruptcy),
                                      hftOption(invert_hft_decision));

        hft_display_filter hdf;
        hdf.display(simulation.get_result());

    }
    catch (const std::exception &e)
    {
        hft_log(ERROR) << e.what();

        return 1;
    }

    return 0;
}

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

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/program_options.hpp>

#include <hft2ctrader_config.hpp>
#include <ctrader_ssl_connection.hpp>
#include <ctrader_api.hpp>

#include <easylogging++.h>

namespace prog_opts = boost::program_options;

static struct hft2ctrade_history_ticks_options_type
{
    std::string config_file_name;
    std::string instrument;
    int begin_week;
    int end_week;
    std::string broker;

} hft2ctrade_history_ticks_options;

#define hft2ctraderOption(__X__) \
    hft2ctrade_history_ticks_options.__X__

#define hft2ctrader_log(__X__) \
    CLOG(__X__, "history_ticks")

int history_ticks_main(int argc, char *argv[])
{

    //
    // Parsing options for history-ticks utility.
    //

    prog_opts::options_description hidden("Hidden options");
    hidden.add_options()
        ("history-ticks", "")
    ;

    prog_opts::options_description desc("Options for history-ticks");
    desc.add_options()
        ("help,h", "Produce help message")
        ("config,c", prog_opts::value<std::string>(&hft2ctraderOption(config_file_name) ) -> default_value("/etc/hft/hft-config.xml"), "Configuration file name")
        ("instrument,i", prog_opts::value<std::string>(&hft2ctraderOption(instrument) ), "Financial instrument ticker – must be supported by specified broker")
        ("begin-week,B", prog_opts::value<int>(&hft2ctraderOption(begin_week) ), "Start week for which data is to be downloaded (inclusive)")
        ("end-week,E", prog_opts::value<int>(&hft2ctraderOption(end_week) ), "End week for which data is to be downloaded (inclusive)")
        ("broker,b", prog_opts::value<std::string>(&hft2ctraderOption(broker)), "Broker being a market gateway")
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

    hft2ctrader_config cfg { hft2ctraderOption(config_file_name), hft2ctraderOption(broker) };

    //
    // Update config with user options.
    //

    cfg.set_instrument(hft2ctraderOption(instrument));
    cfg.set_begin_week(hft2ctraderOption(begin_week));
    cfg.set_end_week(hft2ctraderOption(end_week));

    //
    // Define default configuration for all future bridge loggers.
    //

    el::Configurations logger_cfg;
    logger_cfg.setToDefault();
    logger_cfg.parseFromText(cfg.get_logging_config_utility());
    el::Loggers::setDefaultConfigurations(logger_cfg);

    START_EASYLOGGINGPP(argc, argv);

    //
    // Setup logging.
    //

    el::Logger *logger = el::Loggers::getLogger("history_ticks", true);

    boost::asio::io_context ioctx;

    //
    // Register signal handlers so that the process may be shut down gracefully.
    //

    boost::asio::signal_set signals(ioctx, SIGINT, SIGTERM);
    signals.async_wait(boost::bind(&boost::asio::io_context::stop, &ioctx));

    try
    {
        ctrader_ssl_connection broker_connection {ioctx, cfg};
        ctrader_api api { broker_connection };

    // something.
    }
    catch (const std::exception &e)
    {
        std::cerr << "**** FATAL ERROR: "
                  << e.what() << std::endl;

        return 1;
    }

    std::cout << "*** Completed.";

    return 0;
}

/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System ® ≣≡=-              **
**                                                                    **
**          Copyright © 2017 - 2024 by LLG Ryszard Gradowski          **
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

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/program_options.hpp>

#include <hft2ctrader_config.hpp>
#include <bridge.hpp>

#include <easylogging++.h>

namespace prog_opts = boost::program_options;

static struct hft2ctrade_bridge_options_type
{
    std::string config_file_name;
    std::string hft_ipc_host;
    int hft_ipc_port;
    std::string log_severity;
    std::string broker;

} hft2ctrade_bridge_options;

#define hft2ctraderOption(__X__) \
    hft2ctrade_bridge_options.__X__

#define hft2ctrader_log(__X__) \
    CLOG(__X__, "bridge")

static std::string get_default_hft_ipc_host(void)
{
    auto p = getenv("HFT_ENDPOINT");

    if (p == nullptr)
    {
        return std::string("127.0.0.1");
    }

    return std::string(p);
}

static int get_default_hft_ipc_port(void)
{
    auto p = getenv("HFT_PORT");

    if (p == nullptr)
    {
        return 8137;
    }

    return std::stoi(p);
}

int hft2ctrade_bridge_main(int argc, char *argv[])
{
    //
    // Parsing options for hft2ctrader bridge.
    //

    prog_opts::options_description hidden("Hidden options");
    hidden.add_options()
        ("bridge", "")
    ;

    prog_opts::options_description desc("Options for hft2ctrader bridge");
    desc.add_options()
        ("help,h", "Produce help message")
        ("config,c", prog_opts::value<std::string>(&hft2ctraderOption(config_file_name) ) -> default_value("/etc/hft/hft-config.xml"), "Configuration file name")
        ("hft-ipc-endpoint,e", prog_opts::value<std::string>(&hft2ctraderOption(hft_ipc_host) ) -> default_value(get_default_hft_ipc_host()), "HFT IPC connection endpoint")
        ("hft-ipc-port,p", prog_opts::value<int>(&hft2ctraderOption(hft_ipc_port) ) -> default_value(get_default_hft_ipc_port()), "HFT IPC connection port")
        ("log-severity,L", prog_opts::value<std::string>(&hft2ctraderOption(log_severity) ) -> default_value("INFO"), "Available: FATAL, ERROR, WARNING, INFO, TRACE, DEBUG")
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

    cfg.set_hft_host(hft2ctraderOption(hft_ipc_host));
    cfg.set_hft_port(hft2ctraderOption(hft_ipc_port));

    //
    // Define default configuration for all future bridge loggers.
    //

    el::Configurations logger_cfg;
    logger_cfg.setToDefault();
    logger_cfg.parseFromText(cfg.get_logging_config_bridge(hft2ctraderOption(log_severity)).c_str());
    el::Loggers::setDefaultConfigurations(logger_cfg);

    START_EASYLOGGINGPP(argc, argv);

    //
    // Setup logging.
    //

    el::Logger *logger = el::Loggers::getLogger("bridge", true);

    boost::asio::io_context ioctx;

    //
    // Register signal handlers so that the process may be shut down gracefully.
    //

    boost::asio::signal_set signals(ioctx, SIGINT, SIGTERM);
    signals.async_wait(boost::bind(&boost::asio::io_context::stop, &ioctx));

    try
    {
        #ifdef HFT2CTRADER_TEST

        hft2ctrader_log(INFO) << "Client ID: " << cfg.get_auth_client_id();
        hft2ctrader_log(INFO) << "Client secret: " << cfg.get_auth_client_secret();
        hft2ctrader_log(INFO) << "Access token: " << cfg.get_auth_access_token();
        hft2ctrader_log(INFO) << "Account ID: " << cfg.get_auth_account_id();
        if (cfg.get_account_type() == hft2ctrader_config::account_type::DEMO_ACCOUNT)
        {
            hft2ctrader_log(INFO) << "Account type: DEMO";
        }
        else if (cfg.get_account_type() == hft2ctrader_config::account_type::LIVE_ACCOUNT)
        {
            hft2ctrader_log(INFO) << "Account type: LIVE";
        }
        hft2ctrader_log(INFO) << "Session ID: " << cfg.get_session_id();

        for (auto &i : cfg.get_instruments())
        {
            hft2ctrader_log(INFO) << "Instrument: " << i;
        }

        #endif // HFT2CTRADER_TEST

        bridge hft2ctrader {ioctx, cfg};

        ioctx.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "**** FATAL ERROR: "
                  << e.what() << std::endl;

        hft2ctrader_log(ERROR) << "**** FATAL ERROR: " << e.what();

        el::base::debug::StackTrace();

        return 1;
    }

    hft2ctrader_log(INFO) << "*** Bridge process terminated.";

    return 0;
}

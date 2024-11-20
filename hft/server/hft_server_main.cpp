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

#include <cstdlib>
#include <memory>

#include <daemon_process.hpp>
#include <marketplace_gateway_process.hpp>
#include <hft_session.hpp>
#include <basic_tcp_server.hpp>
#include <hft_server_config.hpp>
#include <deallocator.hpp>

#include <boost/asio.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/program_options.hpp>

#include <easylogging++.h>

namespace prog_opts = boost::program_options;

class hft_server : public basic_tcp_server<hft_session>
{
public:

    using basic_tcp_server<hft_session>::basic_tcp_server;

    ~hft_server(void)
    {
        //
        // Destroy pending sessions, if any.
        //

        deallocator::cleanup();
    }
};

static struct hft_server_options_type
{
    std::string config_file_name;
    bool start_as_daemon;
    std::string ipc_endpoint;
    int ipc_port;

} hft_server_options;

#define hftOption(__X__) \
    hft_server_options.__X__

#define hft_log(__X__) \
    CLOG(__X__, "server")

static std::unique_ptr<daemon_process> server_daemon;

int hft_server_main(int argc, char *argv[])
{
    //
    // Parsing options for hft server.
    //

    prog_opts::options_description hidden("Hidden options");
    hidden.add_options()
        ("server", "")
    ;

    prog_opts::options_description desc("Options for hft server");
    desc.add_options()
        ("help,h", "Produce help message")
        ("daemon,D", "Start server as a daemon")
        ("config,c", prog_opts::value<std::string>(&hftOption(config_file_name) ) -> default_value("/etc/hft/hft-config.xml"), "Server configuration file name")
        ("ipc-endpoint,e", prog_opts::value<std::string>(&hftOption(ipc_endpoint) ) -> default_value("127.0.0.1"), "IPC listen endpoint")
        ("ipc-port,p", prog_opts::value<int>(&hftOption(ipc_port) ) -> default_value(8137), "IPC listen port")
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

    hft_server_config hft_srv_cfg(hftOption(config_file_name));

    //
    // Define default configuration for all future server loggers.
    //

    el::Configurations logger_cfg;
    logger_cfg.setToDefault();
    logger_cfg.parseFromText(hft_srv_cfg.get_logging_config().c_str());
    el::Loggers::setDefaultConfigurations(logger_cfg);

    START_EASYLOGGINGPP(argc, argv);

    //
    // Setup logging.
    //

    el::Logger *logger = el::Loggers::getLogger("server", true);

    //
    // Initial SMS critical alert messanger.
    //

    sms::initialize_sms_alert(hft_srv_cfg.get_sms_alert_config());

    //
    // Should the HFT server be demonized
    // the old way - bypassing systemd.
    //

    if (vm.count("daemon"))
    {
        hftOption(start_as_daemon) = true;
    }
    else
    {
        hftOption(start_as_daemon) = false;
    }

    //
    // Set environment variables for
    // possible use by child processes.
    //

    setenv("HFT_ENDPOINT", hftOption(ipc_endpoint).c_str(), 1);
    setenv("HFT_PORT", std::to_string(hftOption(ipc_port)).c_str(), 1);

    if (hftOption(start_as_daemon))
    {
        try
        {
            server_daemon.reset(new daemon_process("/var/run/hft-server.pid"));
        }
        catch (const daemon_process::exception &e)
        {
            std::cerr << "**** FATAL ERROR: "
                      << e.what() << std::endl;

            hft_log(ERROR) << "**** FATAL ERROR: " << e.what();

            return 1; /* Return with fatal error exit code. */
        }
    }

    boost::asio::io_context ioctx;

    //
    // Register signal handlers so that the daemon may be shut down.
    //

    boost::asio::signal_set signals(ioctx, SIGINT, SIGTERM);
    signals.async_wait(boost::bind(&boost::asio::io_context::stop, &ioctx));

    try
    {
        marketplace_gateway_process gateway_process(ioctx, hftOption(config_file_name));

        hft_server server(ioctx, hftOption(ipc_endpoint), hftOption(ipc_port));

        if (hftOption(start_as_daemon))
        {
            //
            // Notify parent process that daemon is ready to go.
            //

            server_daemon -> notify_success();
        }

        ioctx.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "**** FATAL ERROR: "
                  << e.what() << std::endl;

        el::base::debug::StackTrace();

        hft_log(ERROR) << "**** Goining to terminate server due to FATAL: " << e.what();

        sms::alert(std::string("terminated server: ") + e.what());

        return 1;
    }

    hft_log(WARNING) << "*** Server terminated.";

    return 0;
}

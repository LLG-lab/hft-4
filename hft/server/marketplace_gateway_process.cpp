/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System  ≣≡=-              **
**                                                                    **
**          Copyright  2017 - 2021 by LLG Ryszard Gradowski          **
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

#include <unistd.h>
#include <fstream>
#include <sstream>

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <rapidxml-1.13/rapidxml.hpp>

#include <marketplace_gateway_process.hpp>
#include <utilities.hpp>

#include <easylogging++.h>

#undef TEST

#ifdef TEST
#include <iostream>
#endif

#define hft_log(__X__) \
    CLOG(__X__, "gateway")

namespace fs = boost::filesystem;

marketplace_gateway_process::marketplace_gateway_process(boost::asio::io_context &ioctx, const std::string &config_xml)
    : ioctx_(ioctx)
{
    el::Loggers::getLogger("gateway", true);

    std::string log_file_name = marketplace_gateway_process::prepare_log_file();

    parse_proc_list_xml(hft::utils::file_get_contents(config_xml));

    for (auto &proc_item : process_list_)
    {
        hft_log(INFO) << "Starting external gateway ‘"
                      << proc_item.label << "’";

        boost::filesystem::path path_find = proc_item.program;
        auto start_program = boost::process::search_path(path_find);

        if (start_program.empty())
        {
            std::string err_msg = std::string("Unable to find program to start ‘")
                                  + proc_item.program + std::string("’");

            throw exception(err_msg);
        }
        else
        {
            hft_log(INFO) << "Found bridge application: ‘" << start_program << "’";
        }

        proc_item.child.reset(
            new boost::process::child(
                    boost::process::exe=start_program.c_str(),
                    boost::process::args=proc_item.argv,
                    (boost::process::std_out & boost::process::std_err) > log_file_name,
                    boost::process::start_dir=proc_item.start_dir,
                    boost::process::on_exit=boost::bind(&marketplace_gateway_process::process_exit_notify, this, _1, _2),
                    ioctx_
            )
        );

        std::error_code ec;

        if (! proc_item.child -> running(ec))
        {
            hft_log(ERROR) << "Bridge process failed with error: ‘"
                           << ec << "’";

            throw exception("Failed to start marketplace bridge process");
        }
        else
        {
            hft_log(INFO) << "Bridge process successfully started, " << ec;
        }
    }
}

marketplace_gateway_process::~marketplace_gateway_process(void)
{
    // FIXME: Spróbować najpierw zakończyć proces „po dobremu”
    //        a jesli sie nie zakończy w odpowiednim czasie,
    //        wtedy terminate(). W ten sposób damy szansę na
    //        odpalenie destruktorów w procesie gatewaya.

    for (auto &bpi : process_list_)
    {
        if (bpi.child.use_count() && bpi.child -> running())
        {
            hft_log(INFO) << "Destroying gateway process ‘"
                          << bpi.label << "’";

            bpi.child -> terminate();
            bpi.child -> wait();
        }
    }
}

std::string marketplace_gateway_process::prepare_log_file(void)
{
    std::string regular_log_file = "/var/log/hft/bridge.log";

    auto p = fs::path(regular_log_file);
    boost::system::error_code ec;

    if (fs::exists(p, ec))
    {
        //
        // There is remainder log file from
        // previous session, going to rotate
        //

        fs::rename(p, fs::path(hft::utils::find_free_name(regular_log_file)));
    }

    if (ec)
    {
        hft_log(ERROR) << "Raised system error: ‘" << ec.message() << "’";
    }

    return regular_log_file;
}

void marketplace_gateway_process::parse_proc_list_xml(const std::string &xml_data)
{
    using namespace rapidxml;

    xml_document<> document;

    try
    {
        document.parse<0>(const_cast<char *>(xml_data.c_str()));
    }
    catch (const parse_error &e)
    {
        std::ostringstream error;

        error << e.what() << " here: " << e.where<char>();

        throw exception(error.str());
    }

    xml_node<> *root_node = document.first_node("hft");

    if (root_node == nullptr)
    {
        throw exception("Bad xml config: no ‘hft’ node in file");
    }

    xml_node<> *market_gateway_processes_node = root_node -> first_node("market-gateway-processes");

    if (market_gateway_processes_node == nullptr)
    {
        //
        // There is no node, nothing to parse further.
        //

        return;
    }

    for (xml_node<> *node = market_gateway_processes_node -> first_node(); node; node = node -> next_sibling())
    {
        if (std::string(node -> name()) == "process")
        {
             //
             // Each particular process info processing here.
             //

             bridge_process_info bpi;

             #ifdef TEST
             std::cout << "New ‘process’ node\n";
             #endif

             xml_attribute<> *name_attr = node -> first_attribute("name");

             if (name_attr == nullptr)
             {
                  throw exception("Missing ‘name’ attribute in ‘process’ node in xml config");
             }

             #ifdef TEST
             std::cout << "\t\tname:   " << hft::utils::expand_env_variable(name_attr -> value()) << "\n";
             #endif

             bpi.label = hft::utils::expand_env_variable(name_attr -> value());

             xml_attribute<> *active_attr = node -> first_attribute("active");

             if (active_attr == nullptr)
             {
                  throw exception("Missing ‘active’ attribute in ‘process’ node in xml config");
             }

             #ifdef TEST
             std::cout << "\t\tactive: " << (active_attr -> value()) << "\n";
             #endif

             std::string active = (active_attr -> value());

             if (active == "true" || active == "yes" || active == "1")
             {
                 //
                 // Nothing to do.
                 //
             }
             else if (active == "false" || active == "no" || active == "0")
             {
                 //
                 // Gateway process inactive, skipping.
                 //

                 continue;
             }
             else
             {
                 std::ostringstream error;

                 error << "Illegal ‘active’ attribute value: ‘" << active
                       << "’ in ‘process’ node in xml config";

                 throw exception(error.str());
             }

             xml_node<> *exec_node = node -> first_node("exec");

             if (exec_node == nullptr)
             {
                 throw exception("Missing ‘exec’ child node in ‘process’ node in xml config");
             }

             #ifdef TEST
             std::cout << "\texec:        " << hft::utils::expand_env_variable(exec_node -> value()) << "\n";
             #endif

             bpi.program = hft::utils::expand_env_variable(exec_node -> value());

             xml_node<> *initial_dir_node = node -> first_node("initial-dir");

             if (initial_dir_node == nullptr)
             {
                 throw exception("Missing ‘initial-dir’ child node in ‘process’ node in xml config");
             }

             #ifdef TEST
             std::cout << "\tinitial-dir: " << hft::utils::expand_env_variable(initial_dir_node -> value()) << "\n";
             #endif

             bpi.start_dir = hft::utils::expand_env_variable(initial_dir_node -> value());

             for (xml_node<> *param_node = node -> first_node("param"); param_node; param_node = param_node -> next_sibling())
             {
                 #ifdef TEST
                 std::cout << "\tparam:   " << hft::utils::expand_env_variable(param_node -> value()) << "\n";
                 #endif

                 bpi.argv.push_back(hft::utils::expand_env_variable(param_node -> value()));
             }

             process_list_.push_back(bpi);
        }
    }
}

void marketplace_gateway_process::process_exit_notify(int, const std::error_code &ec)
{
    for (auto &bpi : process_list_)
    {
        if (bpi.child.use_count() && ! bpi.child -> running())
        {
            hft_log(ERROR) << "Bridge process ‘" << bpi.label
                           << "’ terminated unexpectedly, error: ‘"
                           << ec << "’";
        }
    }

    throw exception("Bridge process terminated unexpectedly");
}

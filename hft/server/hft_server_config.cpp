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

#include <cstring>
#include <stdexcept>
#include <sstream>
#include <rapidxml-1.13/rapidxml.hpp>

#include <hft_server_config.hpp>
#include <utilities.hpp>

hft_server_config::hft_server_config(const std::string &xml_file_name)
    : log_severity_(logging_severity::HFT_SEVERITY_INFO)
{
    using namespace rapidxml;

    std::string xml_data = hft::utils::file_get_contents(xml_file_name);

    xml_document<> document;

    try
    {
        document.parse<0>(const_cast<char *>(xml_data.c_str()));
    }
    catch (const parse_error &e)
    {
        std::ostringstream error;

        error << e.what() << " here: " << e.where<char>();

        throw std::runtime_error(error.str());
    }

    xml_node<> *root_node = document.first_node("hft");

    if (root_node == nullptr)
    {
        throw std::runtime_error("Bad xml config: no ‘hft’ node in file");
    }

    xml_node<> *server_node = root_node -> first_node("server");

    if (server_node == nullptr)
    {
        //
        // There is no node, nothing to parse further.
        //

        return;
    }

    for (xml_node<> *node = server_node -> first_node(); node; node = node -> next_sibling())
    {
        if (std::string(node -> name()) == "logging")
        {
             xml_attribute<> *severity_attr = node -> first_attribute("severity");

             if (severity_attr == nullptr)
             {
                  throw std::runtime_error("Missing ‘severity’ attribute in ‘logging’ node in xml config");
             }

             std::string severity_str = (severity_attr -> value());

             if (strcasecmp(severity_attr -> value(), "FATAL") == 0)
             {
                 log_severity_ = logging_severity::HFT_SEVERITY_FATAL;
             }
             else if (strcasecmp(severity_attr -> value(), "ERROR") == 0)
             {
                 log_severity_ = logging_severity::HFT_SEVERITY_ERROR;
             }
             else if (strcasecmp(severity_attr -> value(), "WARNING") == 0)
             {
                 log_severity_ = logging_severity::HFT_SEVERITY_WARNING;
             }
             else if (strcasecmp(severity_attr -> value(), "INFO") == 0)
             {
                 log_severity_ = logging_severity::HFT_SEVERITY_INFO;
             }
             else if (strcasecmp(severity_attr -> value(), "TRACE") == 0)
             {
                 log_severity_ = logging_severity::HFT_SEVERITY_TRACE;
             }
             else if (strcasecmp(severity_attr -> value(), "DEBUG") == 0)
             {
                 log_severity_ = logging_severity::HFT_SEVERITY_DEBUG;
             }
             else
             {
                 std::ostringstream error;

                 error << "Invalid severity attribute ‘"
                       << severity_attr -> value() << "’";

                 throw std::runtime_error(error.str());
             }
        }
    }
}

std::string hft_server_config::get_logging_config(void) const
{
    std::ostringstream logconf;

    logconf << "* GLOBAL:\n"
            << " FORMAT               =  \"%datetime %level [%logger] %msg\"\n"
            << " FILENAME             =  \"/var/log/hft/server.log\"\n"
            << " ENABLED              =  true\n"
            << " TO_FILE              =  true\n"
            << " TO_STANDARD_OUTPUT   =  false\n"
            << " SUBSECOND_PRECISION  =  1\n"
            << " PERFORMANCE_TRACKING =  true\n"
            << " MAX_LOG_FILE_SIZE    =  10485760 ## 10MiB\n"
            << " LOG_FLUSH_THRESHOLD  =  1 ## Flush after every single log\n"
            << "* FATAL:\n"
            << " ENABLED              =  true\n"
            << "* ERROR:\n"
            << " ENABLED              =  " << (log_severity_ >= logging_severity::HFT_SEVERITY_ERROR ? "true\n" : "false\n")
            << "* WARNING:\n"
            << " ENABLED              =  " << (log_severity_ >= logging_severity::HFT_SEVERITY_WARNING ? "true\n" : "false\n")
            << "* INFO:\n"
            << " ENABLED              =  " << (log_severity_ >= logging_severity::HFT_SEVERITY_INFO ? "true\n" : "false\n")
            << "* TRACE:\n"
            << " ENABLED              =  " << (log_severity_ >= logging_severity::HFT_SEVERITY_TRACE ? "true\n" : "false\n")
            << "* DEBUG:\n"
            << " ENABLED              =  " << (log_severity_ >= logging_severity::HFT_SEVERITY_DEBUG ? "true\n" : "false\n");

    return logconf.str();
}

/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System ® ≣≡=-              **
**                                                                    **
**          Copyright © 2017 - 2023 by LLG Ryszard Gradowski          **
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
        if (std::string(node -> name()) == "sms-alerts")
        {
            //
            // Stuff to parse:
            //
            //    <sms-alerts active="false" sandbox="false">
            //        <auth>
            //            <login>dupa</login>
            //            <password>kupa</password>
            //        </auth>
            //
            //        <recipient>724999634</recipient>
            //    </sms-alerts>
            //

            //
            // Obtain ‘active’ attribute.
            //

            xml_attribute<> *active_attr = node -> first_attribute("active");

            if (active_attr == nullptr)
            {
                throw std::runtime_error("Missing ‘active’ attribute in ‘sms-alerts’ node in xml config");
            }

            if (strcasecmp(active_attr -> value(), "FALSE") == 0 ||
                    strcasecmp(active_attr -> value(), "NO") == 0 ||
                        strcasecmp(active_attr -> value(), "0") == 0)
            {
                // Nothing to do.
                continue;
            }
            else if (strcasecmp(active_attr -> value(), "TRUE") == 0 ||
                        strcasecmp(active_attr -> value(), "YES") == 0 ||
                            strcasecmp(active_attr -> value(), "1") == 0)
            {
                sms_config_.enabled = true;
            }
            else
            {
                throw std::runtime_error("Illegal ‘active’ attribute value in ‘sms-alerts’ node in xml config");
            }

            //
            // Obtain ‘sandbox’ attribute, if present.
            //

            xml_attribute<> *sandbox_attr = node -> first_attribute("sandbox");

            if (sandbox_attr != nullptr)
            {
                if (strcasecmp(sandbox_attr -> value(), "FALSE") == 0 ||
                        strcasecmp(sandbox_attr -> value(), "NO") == 0 ||
                            strcasecmp(sandbox_attr -> value(), "0") == 0)
                {
                    sms_config_.sandbox = false;
                }
                else if (strcasecmp(sandbox_attr -> value(), "TRUE") == 0 ||
                             strcasecmp(sandbox_attr -> value(), "YES") == 0 ||
                                 strcasecmp(sandbox_attr -> value(), "1") == 0)
                {
                    sms_config_.sandbox = true;
                }
                else
                {
                    throw std::runtime_error("Illegal ‘sandbox’ attribute value in ‘sms-alerts’ node in xml config");
                }
            }

            //
            // Go into auth node.
            //

            xml_node<> *auth_node = node -> first_node("auth");

            if (auth_node == nullptr)
            {
                throw std::runtime_error("Missing ‘auth’ node in ‘sms-alerts’ node in xml config");
            }

            //
            // Go into auth.login node.
            //

            xml_node<> *auth_login_node = auth_node -> first_node("login");

            if (auth_login_node == nullptr)
            {
                throw std::runtime_error("Missing ‘login’ node in ‘auth’ node in ‘sms-alerts’ node in xml config");
            }
            else
            {
                sms_config_.login = auth_login_node -> value();
            }

            //
            // Go into auth.password node.
            //

            xml_node<> *auth_password_node = auth_node -> first_node("password");

            if (auth_password_node == nullptr)
            {
                throw std::runtime_error("Missing ‘password’ node in ‘auth’ node in ‘sms-alerts’ node in xml config");
            }
            else
            {
                sms_config_.password = auth_password_node -> value();
            }

            //
            // Obtain recipients.
            //

            for (xml_node<> *recipient_node = node -> first_node("recipient"); recipient_node; recipient_node = recipient_node -> next_sibling("recipient"))
            {
                sms_config_.recipients.emplace_back(recipient_node -> value());
            }
        }
        else if (std::string(node -> name()) == "logging")
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

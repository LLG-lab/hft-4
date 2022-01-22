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
#include <fstream>

#include <rapidxml-1.13/rapidxml.hpp>

#include <hft2ctrader_bridge_config.hpp>

hft2ctrader_bridge_config::logging_severity_type hft2ctrader_bridge_config::severity_str2severity_type(const std::string &s)
{
    if (s == "FATAL")   return logging_severity_type::FATAL;
    if (s == "ERROR")   return logging_severity_type::ERROR;
    if (s == "WARNING") return logging_severity_type::WARNING;
    if (s == "INFO")    return logging_severity_type::INFO;
    if (s == "TRACE")   return logging_severity_type::TRACE;
    if (s == "DEBUG")   return logging_severity_type::DEBUG;

    std::string error = "Illegal logging severity identifier ‘" + s + "’";

    throw std::runtime_error(error);
}

std::string hft2ctrader_bridge_config::get_logging_config(const std::string &severity_str) const
{
    auto severity = hft2ctrader_bridge_config::severity_str2severity_type(severity_str);

    std::ostringstream logconf;

    logconf << "* GLOBAL:\n"
            << " FORMAT               =  \"" << broker_ <<", %datetime %level [%logger] %msg\"\n"
            << " ENABLED              =  true\n"
            << " TO_FILE              =  false\n"
            << " TO_STANDARD_OUTPUT   =  true\n"
            << " SUBSECOND_PRECISION  =  1\n"
            << " PERFORMANCE_TRACKING =  true\n"
            << " MAX_LOG_FILE_SIZE    =  10485760 ## 10MiB\n"
            << " LOG_FLUSH_THRESHOLD  =  1 ## Flush after every single log\n"
            << "* FATAL:\n"
            << " ENABLED              =  true\n"
            << "* ERROR:\n"
            << " ENABLED              =  " << (severity >= logging_severity_type::ERROR ? "true\n" : "false\n")
            << "* WARNING:\n"
            << " ENABLED              =  " << (severity >= logging_severity_type::WARNING ? "true\n" : "false\n")
            << "* INFO:\n"
            << " ENABLED              =  " << (severity >= logging_severity_type::INFO ? "true\n" : "false\n")
            << "* TRACE:\n"
            << " ENABLED              =  " << (severity >= logging_severity_type::TRACE ? "true\n" : "false\n")
            << "* DEBUG:\n"
            << " ENABLED              =  " << (severity >= logging_severity_type::DEBUG ? "true\n" : "false\n");

    return logconf.str();
}

void hft2ctrader_bridge_config::xml_parse(const std::string &xml_file_name)
{
    using namespace rapidxml;

    //
    // Load xml file.
    //

    std::fstream in_stream;
    std::string line, xml_data;

    in_stream.open(xml_file_name, std::fstream::in);

    if (in_stream.fail())
    {
        std::string msg = "Unable to open file: " + xml_file_name;

        throw std::runtime_error(msg);
    }

    while (! in_stream.eof())
    {
        std::getline(in_stream, line);
        xml_data += (line + std::string("\n"));
    }

    in_stream.close();

    //
    // Parse xml.
    //

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

    //
    // Traverse through nodes.
    //

    xml_node<> *root_node = document.first_node("hft");

    if (root_node == nullptr)
    {
        throw std::runtime_error("Bad xml config: no ‘hft’ node in file");
    }

    //
    // <market bridge="IC Markets" sessid="icmarkets-session" >
	//     <auth account="demo">
    //         <client-id>XXXXXXX</client-id>
    //         <client-secret>XXXXXX</client-secret>
    //         <access-token>XXXXXX</access-token>
    //         <account-id>0000000</account-id>
    //     </auth>
    //
    //     <instrument ticker="EURUSD"/>
    // </market>
    //

    bool found_config = false;

    for (xml_node<> *node = root_node -> first_node(); node; node = node -> next_sibling())
    {
        if (std::string(node -> name()) == "market")
        {
            xml_attribute<> *bridge_attr = node -> first_attribute("bridge");

            if (bridge_attr == nullptr)
            {
                std::ostringstream error;

                error << "Missing ‘bridge’ attribute in ‘market’ node in xml config: ‘"
                      << xml_file_name << "’";

                throw std::runtime_error(error.str());
            }

            if (std::string(bridge_attr -> value()) != broker_)
            {
                continue;
            }

            xml_attribute<> *sessid_attr = node -> first_attribute("sessid");

            if (sessid_attr == nullptr)
            {
                std::ostringstream error;

                error << "Missing ‘sessid’ attribute in ‘market’ node in xml config: ‘"
                      << xml_file_name << "’";

                throw std::runtime_error(error.str());
            }

            session_id_ = sessid_attr -> value();

            bool found_auth = false;

            for (xml_node<> *market_node = node -> first_node(); market_node; market_node = market_node -> next_sibling())
            {
                std::string node_name = market_node -> name();

                if (node_name == "auth")
                {
                    xml_parse_auth(market_node, xml_file_name);
                    found_auth = true;
                }

                if (node_name == "instrument")
                {
                    xml_parse_instrument(market_node, xml_file_name);
                }
            }

            if (! found_auth)
            {
                std::ostringstream error;

                error << "Missing ‘auth’ settings in market configuration ‘"
                      << broker_ << "’ in xml config: ‘"
                      << xml_file_name << "’";

                throw std::runtime_error(error.str());
            }

            found_config = true;

            break;
        }
    }

    if (! found_config)
    {
        std::ostringstream error;

        error << "Missing configuration for market ‘"
              << broker_ << "’ in xml config: ‘"
              << xml_file_name << "’";

        throw std::runtime_error(error.str());
    }
}

void hft2ctrader_bridge_config::xml_parse_auth(void *node, const std::string &xml_file_name)
{
    using namespace rapidxml;

    xml_node<> *auth_node = reinterpret_cast<xml_node<> *>(node);

    xml_attribute<> *account_attr = auth_node -> first_attribute("account");

    if (account_attr == nullptr)
    {
        std::ostringstream error;

        error << "Missing ‘account’ attribute in ‘auth’ node in xml config: ‘"
              << xml_file_name << "’";

        throw std::runtime_error(error.str());
    }

    if (strcasecmp(account_attr -> value(), "DEMO") == 0)
    {
        account_ = account_type::DEMO_ACCOUNT;
    }
    else if (strcasecmp(account_attr -> value(), "LIVE") == 0)
    {
        account_ = account_type::LIVE_ACCOUNT;
    }
    else
    {
        std::ostringstream error;

        error << "Illegal account type ‘" << account_attr -> value()
              << "’ in ‘auth’ node in xml config: ‘"
              << xml_file_name << "’";

        throw std::runtime_error(error.str());
    }

    xml_node<> *client_id_node = auth_node -> first_node("client-id");

    if (client_id_node == nullptr)
    {
        std::ostringstream error;

        error << "Missing ‘client-id’ in ‘auth’ node in xml config: ‘"
              << xml_file_name << "’";

        throw std::runtime_error(error.str());
    }

    xml_node<> *client_secret_node = auth_node -> first_node("client-secret");

    if (client_secret_node == nullptr)
    {
        std::ostringstream error;

        error << "Missing ‘client-secret’ in ‘auth’ node in xml config: ‘"
              << xml_file_name << "’";

        throw std::runtime_error(error.str());
    }

    xml_node<> *access_token_node = auth_node -> first_node("access-token");

    if (access_token_node == nullptr)
    {
        std::ostringstream error;

        error << "Missing ‘access-token’ in ‘auth’ node in xml config: ‘"
              << xml_file_name << "’";

        throw std::runtime_error(error.str());
    }

    xml_node<> *account_id_node = auth_node -> first_node("account-id");

    if (account_id_node == nullptr)
    {
        std::ostringstream error;

        error << "Missing ‘account-id’ in ‘auth’ node in xml config: ‘"
              << xml_file_name << "’";

        throw std::runtime_error(error.str());
    }

    auth_client_id_ = client_id_node -> value();
    auth_client_secret_ = client_secret_node -> value();
    auth_access_token_ = access_token_node -> value();
    auth_account_id_ = std::stoi(std::string(account_id_node -> value()));
}

void hft2ctrader_bridge_config::xml_parse_instrument(void *node, const std::string &xml_file_name)
{
    using namespace rapidxml;

    xml_node<> *instrument_node = reinterpret_cast<xml_node<> *>(node);

    xml_attribute<> *ticker_attr = instrument_node -> first_attribute("ticker");

    if (ticker_attr == nullptr)
    {
        std::ostringstream error;

        error << "Missing ‘ticker’ attribute in ‘instrument’ node in xml config: ‘"
              << xml_file_name << "’";

        throw std::runtime_error(error.str());
    }

    instruments_.emplace_back(ticker_attr -> value());
}

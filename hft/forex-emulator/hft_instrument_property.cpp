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

#include <stdexcept>
#include <sstream>

#include <rapidxml-1.13/rapidxml.hpp>

#include <utilities.hpp>
#include <hft_instrument_property.hpp>

//
// Constructor load XML configuration file and parses
// following (example) fragment:
//
//	<forex-emulator>
//		<instrument-property ticker="EUR/USD"
//				pip-significant-digit="4"
//				pip-value-per-lot="0.0003876"
//				long-dayswap-per-lot="-0.0001558"
//				short-dayswap-per-lot="0.0000038"
//				commision-per-lot="0.0001649"
//              margin-required-per-lot="0.155"
//		/>
//	</forex-emulator>
//

hft_instrument_property::hft_instrument_property(const std::string &instrument,
                                                     const std::string &config_file_name)
{
    using namespace rapidxml;

    std::string xml_data = hft::utils::file_get_contents(config_file_name);
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
        std::ostringstream error;

        error << "Bad xml config: no ‘hft’ node in file ‘"
              << config_file_name << "’";

        throw std::runtime_error(error.str());
    }

    xml_node<> *forex_emulator_node = root_node -> first_node("forex-emulator");

    if (forex_emulator_node == nullptr)
    {
        std::ostringstream error;

        error << "Cannot find ‘forex-emulator’ node in ‘"
              << config_file_name << "’";

        throw std::runtime_error(error.str());
    }

    for (xml_node<> *node = forex_emulator_node -> first_node(); node; node = node -> next_sibling())
    {
        if (std::string(node -> name()) == "instrument-property")
        {
            xml_attribute<> *ticker_attr = node -> first_attribute("ticker");

            if (ticker_attr == nullptr)
            {
                std::ostringstream error;

                error << "Missing ‘ticker’ attribute in ‘instrument-property’ node in ‘"
                      << config_file_name << "’ file";

                throw std::runtime_error(error.str());
            }

            if (std::string(ticker_attr -> value()) == instrument)
            {
                //
                // Get ‘pip-significant-digit’.
                //

                xml_attribute<> *pip_significant_digit_attr = node -> first_attribute("pip-significant-digit");

                if (pip_significant_digit_attr == nullptr)
                {
                    std::ostringstream error;

                    error << "Missing ‘pip-significant-digit’ attribute in ‘instrument-property’ node in ‘"
                          << config_file_name << "’ file";

                    throw std::runtime_error(error.str());
                }

                std::string psd = pip_significant_digit_attr -> value();

                if (psd.length() != 1 || psd.at(0) <= '0' || psd.at(0) > '9')
                {
                    std::ostringstream error;

                    error << "Illegal value ‘" << psd << "’ of attribute ‘pip-significant-digit’ in ‘instrument-property’ node in ‘"
                          << config_file_name << "’ file";

                    throw std::runtime_error(error.str());
                }

                pip_significant_digit_ = psd.at(0);

                //
                // Get ‘pip-value-per-lot’.
                //

                xml_attribute<> *pip_value_per_lot_attr = node -> first_attribute("pip-value-per-lot");

                if (pip_value_per_lot_attr == nullptr)
                {
                    std::ostringstream error;

                    error << "Missing ‘pip-value-per-lot’ attribute in ‘instrument-property’ node in ‘"
                          << config_file_name << "’ file";

                    throw std::runtime_error(error.str());
                }

                pip_value_per_lot_ = std::stod(std::string(pip_value_per_lot_attr -> value()));

                //
                // Get ‘long-dayswap-per-lot’.
                //

                xml_attribute<> *long_dayswap_per_lot_attr = node -> first_attribute("long-dayswap-per-lot");

                if (long_dayswap_per_lot_attr == nullptr)
                {
                    std::ostringstream error;

                    error << "Missing ‘long-dayswap-per-lot’ attribute in ‘instrument-property’ node in ‘"
                          << config_file_name << "’ file";

                    throw std::runtime_error(error.str());
                }

                long_dayswap_per_lot_ = std::stod(std::string(long_dayswap_per_lot_attr -> value()));

                //
                // Get ‘short-dayswap-per-lot’.
                //

                xml_attribute<> *short_dayswap_per_lot_attr = node -> first_attribute("short-dayswap-per-lot");

                if (short_dayswap_per_lot_attr == nullptr)
                {
                    std::ostringstream error;

                    error << "Missing ‘short-dayswap-per-lot’ attribute in ‘instrument-property’ node in ‘"
                          << config_file_name << "’ file";

                    throw std::runtime_error(error.str());
                }

                short_dayswap_per_lot_ = std::stod(std::string(short_dayswap_per_lot_attr -> value()));

                //
                // Get ‘commision-per-lot’.
                //

                xml_attribute<> *commision_per_lot_attr = node -> first_attribute("commision-per-lot");

                if (commision_per_lot_attr == nullptr)
                {
                    std::ostringstream error;

                    error << "Missing ‘commision-per-lot’ attribute in ‘instrument-property’ node in ‘"
                          << config_file_name << "’ file";

                    throw std::runtime_error(error.str());
                }

                commision_per_lot_ = std::stod(std::string(commision_per_lot_attr -> value()));

                //
                // Get ‘margin-required-per-lot’.
                //

                xml_attribute<> *margin_required_per_lot_attr = node -> first_attribute("margin-required-per-lot");

                if (margin_required_per_lot_attr == nullptr)
                {
                    std::ostringstream error;

                    error << "Missing ‘margin-required-per-lot’ attribute in ‘instrument-property’ node in ‘"
                          << config_file_name << "’ file";

                    throw std::runtime_error(error.str());
                }

                margin_required_per_lot_ = std::stod(std::string(margin_required_per_lot_attr -> value()));

                return;
            }
        }
    }

    std::ostringstream error;

    error << "Cannot find instrument ‘" << instrument
          << "’ property settings for forex emulator in config file ‘"
          << config_file_name << "’";

    throw std::runtime_error(error.str());
}

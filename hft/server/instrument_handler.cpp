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

#include <boost/algorithm/string.hpp>
#include <boost/json.hpp>
#include <boost/dll.hpp>

#include <sms_alert.hpp>
#include <utilities.hpp>
#include <hft_ih_dummy.hpp>

int instrument_handler::floating2pips(double price) const
{
    char pips_digit = (char)(handler_informations_.pips_digit + 48);

    return hft::utils::floating2pips(price, pips_digit);
}

std::string instrument_handler::uid(void)
{
    static char arr[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
                         'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
                         'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
                         'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F',
                         'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
                         'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
                         'W', 'X', 'Y', 'Z', '0', '1', '2', '3',
                         '4', '5', '6', '7', '8', '9'};

    static int index = 0;

    auto millis = hft::utils::get_current_timestamp();

    std::string ret = std::string("hft_") + std::to_string(millis);
    ret.push_back(arr[index++ % sizeof(arr)]);

    return ret;
}

//
// Json-Helper routines implementation.
//

bool instrument_handler::json_exist_attribute(const boost::json::object &obj,
                                                  const std::string &attr_name)
{
    using namespace boost::json;

    if (! obj.contains(attr_name))
    {
        return false;
    }

    return true;
}

bool instrument_handler::json_get_bool_attribute(const boost::json::object &obj, const std::string &attr_name)
{
    using namespace boost::json;

    if (! obj.contains(attr_name))
    {
        std::string error_msg = std::string("Missing attribute ‘")
                                + attr_name + std::string("’");

        throw std::runtime_error(error_msg);
    }

    value const &attr_v = obj.at(attr_name);

    if (attr_v.kind() != kind::bool_)
    {
        std::string error_msg = std::string("Invalid attribute type for ‘")
                                + attr_name + std::string("’ - boolean type expected");

        throw std::runtime_error(error_msg);
    }

    return attr_v.get_bool();
}

double instrument_handler::json_get_double_attribute(const boost::json::object &obj,
                                                         const std::string &attr_name)
{
    using namespace boost::json;

    if (! obj.contains(attr_name))
    {
        std::string error_msg = std::string("Missing attribute ‘")
                                + attr_name + std::string("’");

        throw std::runtime_error(error_msg);
    }

    value const &attr_v = obj.at(attr_name);

    if (attr_v.kind() == kind::double_)
    {
        return attr_v.get_double();
    }
    else if (attr_v.kind() == kind::int64)
    {
        return attr_v.get_int64();
    }
    else
    {
        std::string error_msg = std::string("Invalid attribute type for ‘")
                                + attr_name + std::string("’ - floating point type expected");

        throw std::runtime_error(error_msg);
    }
}

int instrument_handler::json_get_int_attribute(const boost::json::object &obj,
                                                   const std::string &attr_name)
{
    using namespace boost::json;

    if (! obj.contains(attr_name))
    {
        std::string error_msg = std::string("Missing attribute ‘")
                                + attr_name + std::string("’");

        throw std::runtime_error(error_msg);
    }

    value const &attr_v = obj.at(attr_name);

    if (attr_v.kind() != kind::int64)
    {
        std::string error_msg = std::string("Invalid attribute type for ‘")
                                + attr_name + std::string("’ - integer type expected");

        throw std::runtime_error(error_msg);
    }

    return attr_v.get_int64();
}

std::string instrument_handler::json_get_string_attribute(const boost::json::object &obj,
                                                              const std::string &attr_name)
{
    using namespace boost::json;

    if (! obj.contains(attr_name))
    {
        std::string error_msg = std::string("Missing attribute ‘")
                                + attr_name + std::string("’");

        throw std::runtime_error(error_msg);
    }

    value const &attr_v = obj.at(attr_name);

    if (attr_v.kind() != kind::string)
    {
        std::string error_msg = std::string("Invalid attribute type for ‘")
                                + attr_name + std::string("’ - string type expected");

        throw std::runtime_error(error_msg);
    }

    return std::string(attr_v.get_string().c_str());
}

const boost::json::object &instrument_handler::json_get_object_attribute(const boost::json::object &obj, const std::string &attr_name)
{
    using namespace boost::json;

    if (! obj.contains(attr_name))
    {
        std::string error_msg = std::string("Missing attribute ‘")
                                + attr_name + std::string("’");

        throw std::runtime_error(error_msg);
    }

    value const &attr_v = obj.at(attr_name);

    if (attr_v.kind() != kind::object)
    {
        std::string error_msg = std::string("Invalid attribute type for ‘")
                                + attr_name + std::string("’ - object type expected");

        throw std::runtime_error(error_msg);
    }

    return attr_v.get_object();
}

//
// Extra.
//

void instrument_handler::sms_alert(const std::string &message)
{
    sms::alert(message);
}

//
// Plugin support.
//

static std::map<std::string, boost::dll::shared_library> ih_plugins;

static instrument_handler_ptr import_instrument_handler_from_plugin(const std::string &name, const instrument_handler::init_info &handler_info)
{
    if (ih_plugins.find(name) == ih_plugins.end())
    {
        std::string file_name = std::string("/var/lib/hft/instrument-handlers/lib")
                                + name + std::string(".so");

        ih_plugins[name] = boost::dll::shared_library(file_name);

        if (! ih_plugins[name].has("create_plugin"))
        {
             ih_plugins.erase(name);

             std::string error_msg = std::string("Invalid instrument handler plugin ‘")
                                     + file_name + std::string("’ - no factory function");

             throw std::runtime_error(error_msg);
        }
    }

    return ih_plugins[name].get<instrument_handler_ptr(const instrument_handler::init_info &handler_info)>("create_plugin")(handler_info);
}

//
// Instrument handler factory routine.
//

instrument_handler_ptr create_instrument_handler(const std::string &session_dir, const std::string &instrument)
{
    using namespace boost::json;

    instrument_handler::init_info handler_info;

    handler_info.ticker = instrument;
    handler_info.ticker_fmt2 = boost::erase_all_copy(instrument, "/");
    handler_info.work_dir = session_dir + std::string("/") + handler_info.ticker_fmt2;

    std::string manifest = handler_info.work_dir + std::string("/manifest.json");

    std::string json_data = hft::utils::file_get_contents(manifest);

    value jv;

    try
    {
        jv = parse(json_data);
    }
    catch (const system_error &e)
    {
        std::string error_message = std::string("Failed to parse file ") + manifest;

        throw std::runtime_error(error_message);
    }

    if (jv.kind() != kind::object)
    {
        std::string error_message = std::string("Invalid manifest file ") + manifest;

        throw std::runtime_error(error_message);
    }

    object const &obj = jv.get_object();

    if (obj.empty())
    {
        std::string error_message = std::string("Invalid manifest file ") + manifest;

        throw std::runtime_error(error_message);
    }

    if (! obj.contains("instrument"))
    {
        std::string error_message = std::string("Missing ‘instrument’ attribute in manifest file ") + manifest;

        throw std::runtime_error(error_message);
    }

    value const &instrument_v = obj.at("instrument");

    if (instrument_v.kind() != kind::string)
    {
        std::string error_message = std::string("Invalid ‘instrument’ attribute type in manifest file ") + manifest;

        throw std::runtime_error(error_message);
    }

    std::string instr = instrument_v.get_string().c_str();

    if (instr != instrument)
    {
        std::string error_message = std::string("Missmatch ‘instrument’ attribute (")
                                    + instr + std::string(" != ") + instrument
                                    + std::string(") type in manifest file ")
                                    + manifest;

        throw std::runtime_error(error_message);
    }

    //
    // Get instrument description.
    //

    if (! obj.contains("description"))
    {
        std::string error_message = std::string("Missing ‘description’ attribute in manifest file ") + manifest;

        throw std::runtime_error(error_message);
    }

    value const &description_v = obj.at("description");

    if (description_v.kind() != kind::string)
    {
        std::string error_message = std::string("Invalid ‘description’ attribute type in manifest file ") + manifest;

        throw std::runtime_error(error_message);
    }

    handler_info.description = description_v.get_string().c_str();

    //
    // Get pips digit.
    //

    if (! obj.contains("pips_digit"))
    {
        std::string error_message = std::string("Missing ‘pips_digit’ attribute in manifest file ") + manifest;

        throw std::runtime_error(error_message);
    }

    value const &pips_digit_v = obj.at("pips_digit");

    if (pips_digit_v.kind() != kind::int64)
    {
        std::string error_message = std::string("Invalid ‘pips_digit’ attribute type in manifest file ") + manifest;

        throw std::runtime_error(error_message);
    }

    handler_info.pips_digit = pips_digit_v.get_int64();

    if (handler_info.pips_digit <= 0 || handler_info.pips_digit >= 10)
    {
        std::string error_message = std::string("Illegal value ‘")
                                    + std::to_string(handler_info.pips_digit)
                                    + std::string("’ of ‘pips_digit’ attribute in manifest file ")
                                    + manifest;

        throw std::runtime_error(error_message);
    }

    //
    // Get Trade Time Frame - presence not mandatory.
    //

    if (obj.contains("trade_time_frame"))
    {
        value const &ttf_v = obj.at("trade_time_frame");

        if (ttf_v.kind() != kind::object)
        {
            std::string error_message = std::string("Invalid ‘trade_time_frame’ attribute type in manifest file ") + manifest;

            throw std::runtime_error(error_message);
        }

        object const &ttf_obj = ttf_v.get_object();

        handler_info.ttf.init_from_json_object(ttf_obj);
    }

    //
    // Get handler.
    //

    if (! obj.contains("handler"))
    {
        std::string error_message = std::string("Missing ‘handler’ attribute in manifest file ") + manifest;

        throw std::runtime_error(error_message);
    }

    value const &handler_v = obj.at("handler");

    if (handler_v.kind() != kind::string)
    {
        std::string error_message = std::string("Invalid ‘handler’ attribute type in manifest file ") + manifest;

        throw std::runtime_error(error_message);
    }

    std::string handler = handler_v.get_string().c_str();

    //
    // Get handler_options.
    //

    if (! obj.contains("handler_options"))
    {
        std::string error_message = std::string("Missing ‘handler_options’ attribute in manifest file ") + manifest;

        throw std::runtime_error(error_message);
    }

    value const &handler_options_v = obj.at("handler_options");

    if (handler_options_v.kind() != kind::object)
    {
        std::string error_message = std::string("Invalid ‘handler_options’ attribute type in manifest file ") + manifest;

        throw std::runtime_error(error_message);
    }

    instrument_handler_ptr ih = nullptr;

    object const &handler_options_obj = handler_options_v.get_object();

    //
    // Create specific handler.
    //

    if (handler == "dummy")
    {
        ih = new hft_ih_dummy(handler_info);
    }
    else
    {
        //
        // No handler found from set of built-in
        // handlers, trying to import from plugin.
        //

        ih = import_instrument_handler_from_plugin(handler, handler_info);
    }

    ih -> init_handler(handler_options_obj);

    return ih;
}

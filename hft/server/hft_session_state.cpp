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

#include <hft_session_state.hpp>
#include <instrument_handler.hpp>
#include <utilities.hpp>
#include <boost/json.hpp>
#include <easylogging++.h>

#define hft_log(__X__) \
    CLOG(__X__, "session_state")

session_state::session_state(const std::string &session_directory)
    : state_filename_ { session_directory + "/session_state.json" },
      session_directory_ { session_directory },
      mode_ { session_mode::PERSISTENT },
      changed_ { false}
{
    el::Loggers::getLogger("session_state", true);
    load();
}

session_state::~session_state(void)
{
    try
    {
        save();
    }
    catch (...)
    {
        hft_log(ERROR) << "Failed to save " << state_filename_;
    }
}

svr session_state::variable(instrument_handler *ph, const std::string &name, varscope scope)
{
    if (scope == varscope::GLOBAL)
    {
        return svr(changed_, variables_["global"][name]);
    }

    return svr(changed_, variables_[ph -> get_ticker_fmt2()][name]);
}

void session_state::save(void)
{
    using namespace boost::json;

    if (mode_ == session_mode::VOLATILE || ! changed_)
    {
        return;
    }

    //
    // Example structure to create:
    //
    // {
    //     "session_type": "simulation",
    //     "custom_session_variables" : [
    //         {"scope":"global","name":"xgrid.lockedby","value":"USDCHF"},
    //         {"scope":"EURUSD","name":"dupsko","value":"2"}
    //     ]
    // }
    //

    object main_object;

    switch (mode_)
    {
        case session_mode::PERSISTENT:
            main_object["session_type"] = "PERSISTENT";
            break;
        case session_mode::VOLATILE:
            main_object["session_type"] = "VOLATILE";
            break;
    }

    array var_array;
    object var_array_object;

    for (auto &item1 : variables_)
    {
        var_array_object["scope"] = item1.first;

        for (auto &item2 : item1.second)
        {
            var_array_object["name"] = item2.first;
            var_array_object["value"] = item2.second;

            var_array.emplace_back(var_array_object);
        }
    }

    main_object["custom_session_variables"] = var_array;

    std::string data = boost::json::serialize(main_object);

    hft::utils::file_put_contents(state_filename_, data);

    changed_ = false;
}

void session_state::load(void)
{
    using namespace boost::json;

    std::string data;

    try
    {
        data = hft::utils::file_get_contents(state_filename_);
    }
    catch (const std::runtime_error &e)
    {
        hft_log(ERROR) << e.what();

        changed_ = true;

        return;
    }

    value jv;

    try
    {
        jv = parse(data);
    }
    catch (const system_error &e)
    {
        hft_log(ERROR) << "Failed to parse file " << state_filename_
                       << " : " << e.what();

        throw std::runtime_error("Session error");
    }

    if (jv.kind() != kind::object)
    {
        hft_log(ERROR) << "Bad JSON – object expected : "
                       << state_filename_;

        throw std::runtime_error("Session error");
    }

    object const &obj = jv.get_object();

    if (obj.empty())
    {
        hft_log(ERROR) << "Bad JSON – empty data : "
                       << state_filename_;

        throw std::runtime_error("Session error");
    }

    if (! obj.contains("session_type"))
    {
        hft_log(ERROR) << "Bad JSON – missing attribute ‘session_type’ : "
                       << state_filename_;

        throw std::runtime_error("Session error");
    }

    value const &session_type_v = obj.at("session_type");

    if (session_type_v.kind() != kind::string)
    {
        hft_log(ERROR) << "Bad JSON – attribute ‘session_type’ not a string : "
                       << state_filename_;

        throw std::runtime_error("Session error");
    }

    std::string session_type_str = session_type_v.get_string().c_str();

    if (session_type_str == "PERSISTENT")
    {
        mode_ = session_mode::PERSISTENT;
    }
    else if (session_type_str == "VOLATILE")
    {
        mode_ = session_mode::VOLATILE;
    }
    else
    {
        hft_log(ERROR) << "Bad JSON – illegal value of ‘session_type’ : "
                       << state_filename_;

        throw std::runtime_error("Session error");
    }

    //
    // Variables.
    //

    if (! obj.contains("custom_session_variables"))
    {
        hft_log(ERROR) << "Bad JSON – missing attribute ‘custom_session_variables’ : "
                       << state_filename_;

        throw std::runtime_error("Session error");
    }

    value const &custom_session_variables_v = obj.at("custom_session_variables");

    if (custom_session_variables_v.kind() != kind::array)
    {
        hft_log(ERROR) << "Bad JSON – attribute ‘custom_session_variables’ not a array : "
                       << state_filename_;

        throw std::runtime_error("Session error");
    }

    array const &arr = custom_session_variables_v.get_array();

    std::string var_scope, var_name, var_value;

    for (auto it = arr.begin(); it != arr.end(); it++)
    {
        value const &array_item_v = *it;

        if (array_item_v.kind() != kind::object)
        {
            hft_log(ERROR) << "Bad JSON – attribute ‘custom_session_variables’ array item not a object : "
                           << state_filename_;

            throw std::runtime_error("Session error");
        }

        object const &variable_obj = array_item_v.get_object();

        //
        // Scope.
        //

        if (! variable_obj.contains("scope"))
        {
            hft_log(ERROR) << "Bad JSON – missing ‘scope’ attribute in ‘custom_session_variables’ array item : "
                           << state_filename_;

            throw std::runtime_error("Session error");
        }

        value const &scope_v = variable_obj.at("scope");

        if (scope_v.kind() != kind::string)
        {
            hft_log(ERROR) << "Bad JSON – missing ‘scope’ attribute in ‘custom_session_variables’ array item not a string: "
                           << state_filename_;

            throw std::runtime_error("Session error");
        }

        var_scope = scope_v.get_string().c_str();

        //
        // Name.
        //

        if (! variable_obj.contains("name"))
        {
            hft_log(ERROR) << "Bad JSON – missing ‘name’ attribute in ‘custom_session_variables’ array item : "
                           << state_filename_;

            throw std::runtime_error("Session error");
        }

        value const &name_v = variable_obj.at("name");

        if (name_v.kind() != kind::string)
        {
            hft_log(ERROR) << "Bad JSON – missing ‘name’ attribute in ‘custom_session_variables’ array item not a string: "
                           << state_filename_;

            throw std::runtime_error("Session error");
        }

        var_name = name_v.get_string().c_str();

        //
        // Value.
        //

        if (! variable_obj.contains("value"))
        {
            hft_log(ERROR) << "Bad JSON – missing ‘value’ attribute in ‘custom_session_variables’ array item : "
                           << state_filename_;

            throw std::runtime_error("Session error");
        }

        value const &value_v = variable_obj.at("value");

        if (value_v.kind() != kind::string)
        {
            hft_log(ERROR) << "Bad JSON – missing ‘value’ attribute in ‘custom_session_variables’ array item not a string: "
                           << state_filename_;

            throw std::runtime_error("Session error");
        }

        var_value = value_v.get_string().c_str();

        //
        // Put together {scope, name, value} into variable map.
        //

        variables_[var_scope][var_name] = var_value;
    }
}

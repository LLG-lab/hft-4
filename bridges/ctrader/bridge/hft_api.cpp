/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System ® ≣≡=-              **
**                                                                    **
**          Copyright © 2017 - 2026 by LLG Ryszard Gradowski          **
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

#include <boost/json.hpp>

#include <hft_api.hpp>
#include <aux_functions.hpp>

#include <sstream>
#include <stdexcept>

void hft_api::hft_init_session(const std::string &sessid, const instruments_container &instruments)
{
    if (instruments.empty())
    {
        throw std::invalid_argument("hft_api: hft_init_session: Empty instrument set");
    }

    if (sessid.empty())
    {
        throw std::invalid_argument("hft_api: hft_init_session: Empty session id");
    }

    std::string instruments_str;

    for (int i = 0; i < instruments.size() - 1; i++)
    {
        instruments_str += "\"" + instruments[i] + "\",";
    }

    instruments_str += "\"" + instruments[instruments.size() - 1] + "\"";

    //
    // Sending ‘init’ to HFT server.
    //

    std::ostringstream payload;

    payload << "{\"method\":\"init\",\"sessid\":\""
            << sessid << "\",\"instruments\":["
            << instruments_str << "]}\n";

    connection_.send_data(payload.str());
}

void hft_api::hft_sync(const std::string &instrument, unsigned long timestamp, const std::string &identifier, position_type direction, double price, int volume)
{
    if (instrument.empty())
    {
        throw std::invalid_argument("hft_api: hft_sync: Empty instrument");
    }

    if (identifier.empty())
    {
        throw std::invalid_argument("hft_api: hft_sync: Empty position identifier");
    }

    std::string direction_str;

    switch (direction)
    {
        case position_type::LONG_POSITION:
            direction_str = "LONG";
            break;
        case position_type::SHORT_POSITION:
            direction_str = "SHORT";
            break;
        default:
            throw std::invalid_argument("hft_api: hft_sync: Illegal trade side");
    }

    std::ostringstream payload;

    payload << "{\"method\":\"sync\",\"instrument\":\""
            << instrument << "\",\"timestamp\":\""
            << aux::timestamp2string(timestamp) << "\",\"id\":\""
            << identifier << "\",\"direction\":\""
            << direction_str << "\",\"price\":"
            << price << ",\"qty\":" << volume << "}\n";

    connection_.send_data(payload.str());
}

void hft_api::hft_send_tick(const std::string &instrument, unsigned long timestamp, double ask, double bid, double equity, double free_margin)
{
    if (instrument.empty())
    {
        throw std::invalid_argument("hft_api: hft_send_tick: Empty instrument");
    }

    std::ostringstream payload;

    payload << "{\"method\":\"tick\",\"instrument\":\""
            << instrument << "\",\"timestamp\":\""
            << aux::timestamp2string(timestamp) << "\",\"ask\":"
            << ask << ",\"bid\":"
            << bid << ",\"equity\":"
            << equity  << ",\"free_margin\":"
            << free_margin << "}\n";

    connection_.send_data(payload.str());
}

void hft_api::hft_send_open_notify(const std::string &instrument, const std::string &identifier, bool status, double price)
{
    if (instrument.empty())
    {
        throw std::invalid_argument("hft_api: hft_send_open_notify: Empty instrument");
    }

    if (identifier.empty())
    {
        throw std::invalid_argument("hft_api: hft_send_open_notify: Empty position identifier");
    }

    std::ostringstream payload;

    std::string s = (status ? "true" : "false");

    payload << "{\"method\":\"open_notify\",\"instrument\":\""
            << instrument << "\",\"id\":\"" << identifier
            << "\",\"status\":" << s << ",\"price\":"
            << price << "}\n";

    connection_.send_data(payload.str());
}

void hft_api::hft_send_close_notify(const std::string &instrument, const std::string &identifier, bool status, double price)
{
    if (instrument.empty())
    {
        throw std::invalid_argument("hft_api: hft_send_close_notify: Empty instrument");
    }

    if (identifier.empty())
    {
        throw std::invalid_argument("hft_api: hft_send_close_notify: Empty position identifier");
    }

    std::ostringstream payload;

    std::string s = (status ? "true" : "false");

    payload << "{\"method\":\"close_notify\",\"instrument\":\""
            << instrument << "\",\"id\":\"" << identifier
            << "\",\"status\":" << s << ",\"price\":"
            << price << "}\n";

    connection_.send_data(payload.str());
}

void hft_api::hft_response::unserialize(const std::string &payload)
{
    using namespace boost::json;

    error_message_.clear();
    instrument_.clear();
    new_positions_info_.clear();
    close_positions_info_.clear();

    value jv;

    try
    {
        jv = parse(payload);
    }
    catch (const system_error &e)
    {
        throw std::invalid_argument("JSON parse error");
    }

    if (jv.kind() != kind::object)
    {
        throw std::invalid_argument("Bad response message");
    }

    object const &obj = jv.get_object();

    if (obj.empty())
    {
        throw std::invalid_argument("Empty response");
    }

    if (! obj.contains("status"))
    {
        throw std::invalid_argument("Missing status");
    }

    value const &status_v = obj.at("status");

    if (status_v.kind() != kind::string)
    {
        throw std::invalid_argument("Invalid status type");
    }

    std::string status = status_v.get_string().c_str();

    if (status == "ack")
    {
        return;
    }
    else if (status == "error")
    {
        if (! obj.contains("message"))
        {
            throw std::invalid_argument("Missing message");
        }

        value const &message_v = obj.at("message");

        if (message_v.kind() != kind::string)
        {
            throw std::invalid_argument("Invalid message type");
        }

        error_message_ = message_v.get_string().c_str();

        return;
    }
    else if (status == "advice")
    {
        if (! obj.contains("instrument"))
        {
            throw std::invalid_argument("Missing instrument");
        }

        value const &instrument_v = obj.at("instrument");

        if (instrument_v.kind() != kind::string)
        {
            throw std::invalid_argument("Invalid instrument type");
        }

        instrument_ = instrument_v.get_string().c_str();

        if (! obj.contains("operations"))
        {
            throw std::invalid_argument("Missing operations");
        }

        value const &operations_v = obj.at("operations");

        if (operations_v.kind() != kind::array)
        {
            throw std::invalid_argument("Invalid operations type");
        }

        array const &arr = operations_v.get_array();

        if (arr.empty())
        {
            throw std::invalid_argument("Empty operations for advice ‘status’");
        }

        for (auto it = arr.begin(); it != arr.end(); it++)
        {
            value const &array_item_v = *it;

            if (array_item_v.kind() != kind::object)
            {
                throw std::invalid_argument("Operation must be an object for advice ‘status’");
            }

            //
            // Obtain object from array.
            //

            object const &operation_obj = array_item_v.get_object();

            if (operation_obj.empty())
            {
                throw std::invalid_argument("Empty operation object");
            }

            if (! operation_obj.contains("op"))
            {
                throw std::invalid_argument("Missing ‘op’ in operation object");
            }

            value const &op_v = operation_obj.at("op");

            if (op_v.kind() != kind::string)
            {
                throw std::invalid_argument("Invalid op type in operation object");
            }

            std::string op = op_v.get_string().c_str();
            position_type pd = position_type::UNDEFINED_POSITION;

            if (op == "close")
            {
                //
                // Handle close and continue.
                //

                if (! operation_obj.contains("id"))
                {
                    throw std::invalid_argument("Missing ‘id’ in operation object");
                }

                value const &id_v = operation_obj.at("id");

                if (id_v.kind() != kind::string)
                {
                    throw std::invalid_argument("Invalid id type in operation object");
                }

                close_positions_info_.emplace_back(id_v.get_string().c_str());

                continue;
            }
            else if (op == "LONG")
            {
                pd = position_type::LONG_POSITION;
            }
            else if (op == "SHORT")
            {
                pd = position_type::SHORT_POSITION;
            }
            else
            {
                std::string err = std::string("Illegal operation ‘")
                                  + op + std::string("’ in operation object");

                throw std::invalid_argument(err);
            }

            //
            // Continue parsing ‘id’ and ‘qty’ attributes
            // for LONG or SHORT operation.
            //

            if (! operation_obj.contains("id"))
            {
                throw std::invalid_argument("Missing ‘id’ in operation object");
            }

            value const &id_v = operation_obj.at("id");

            if (id_v.kind() != kind::string)
            {
                throw std::invalid_argument("Invalid id type in operation object");
            }

            std::string id = id_v.get_string().c_str();

            if (! operation_obj.contains("qty"))
            {
                throw std::invalid_argument("Missing ‘qty’ in operation object");
            }

            value const &qty_v = operation_obj.at("qty");

            double qty;
            if (qty_v.kind() == kind::int64)
            {
                qty = qty_v.get_int64();
            }
            else if (qty_v.kind() == kind::double_)
            {
                qty = qty_v.get_double();
            }
            else
            {
                throw std::invalid_argument("Invalid qty type in operation object");
            }

            new_positions_info_.emplace_back(pd, id, qty);
        }
    }
    else
    {
        std::string err = std::string("Illegal status ‘")
                          + status + std::string("’");

        throw std::invalid_argument(err);
    }
}

/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System ® ≣≡=-              **
**                                                                    **
**          Copyright © 2017 - 2021 by LLG Ryszard Gradowski          **
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

#include <boost/json.hpp>

#include <hft_response.hpp>

namespace hft {
namespace protocol {

std::string response::serialize(void) const
{
    using namespace boost::json;

    std::string ret;

    if (error_message_.empty() && new_positions_.empty() && close_positions_.empty())
    {
        ret = "{\"status\":\"ack\"}\n";

        return ret;
    }

    if (! error_message_.empty())
    {
        object obj;
        obj["status"] = "error";
        obj["message"] = error_message_;

        ret = boost::json::serialize(obj);
        ret += std::string("\n");

        return ret;
    }

    object obj, cp_obj, op_obj;
    obj["status"] = "advice";

    array arr;

    cp_obj["op"] = "close";

    for (auto &item : close_positions_)
    {
        cp_obj["id"] = item;

        arr.emplace_back(cp_obj);
    }

    for (auto &item : new_positions_)
    {
        if (item.pd_ == position_direction::POSITION_LONG)
        {
            op_obj["op"] = "LONG";
        }
        else if (item.pd_ == position_direction::POSITION_SHORT)
        {
            op_obj["op"] = "SHORT";
        }
        else
        {
            op_obj["op"] = "?";
        }

        op_obj["id"]  = item.id_;
        op_obj["qty"] = item.qty_;

        arr.emplace_back(op_obj);
    }

    obj["operations"] = arr;

    ret = boost::json::serialize(obj);
    ret += std::string("\n");

    return ret;
}

void response::unserialize(const std::string &payload)
{
    using namespace boost::json;

    error_message_.clear();
    new_positions_.clear();
    close_positions_.clear();

    value jv;

    try
    {
        jv = parse(payload);
    }
    catch (const system_error &e)
    {
        throw response::violation_error("JSON parse error");
    }

    if (jv.kind() != kind::object)
    {
        throw response::violation_error("Bad response message");
    }

    object const &obj = jv.get_object();

    if (obj.empty())
    {
        throw response::violation_error("Empty response");
    }

    if (! obj.contains("status"))
    {
        throw response::violation_error("Missing status");
    }

    value const &status_v = obj.at("status");

    if (status_v.kind() != kind::string)
    {
        throw response::violation_error("Invalid status type");
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
            throw response::violation_error("Missing message");
        }

        value const &message_v = obj.at("message");

        if (message_v.kind() != kind::string)
        {
            throw response::violation_error("Invalid message type");
        }

        error_message_ = message_v.get_string().c_str();

        return;
    }
    else if (status == "advice")
    {
        if (! obj.contains("operations"))
        {
            throw response::violation_error("Missing operations");
        }

        value const &operations_v = obj.at("operations");

        if (operations_v.kind() != kind::array)
        {
            throw response::violation_error("Invalid operations type");
        }

        array const &arr = operations_v.get_array();

        if (arr.empty())
        {
            throw response::violation_error("Empty operations for advice ‘status’");
        }

        for (auto it = arr.begin(); it != arr.end(); it++)
        {
            value const &array_item_v = *it;

            if (array_item_v.kind() != kind::object)
            {
                throw response::violation_error("Operation must be an object for advice ‘status’");
            }

            //
            // Obtain object from array.
            //

            object const &operation_obj = array_item_v.get_object();

            if (operation_obj.empty())
            {
                throw response::violation_error("Empty operation object");
            }

            if (! operation_obj.contains("op"))
            {
                throw response::violation_error("Missing ‘op’ in operation object");
            }

            value const &op_v = operation_obj.at("op");

            if (op_v.kind() != kind::string)
            {
                throw response::violation_error("Invalid op type in operation object");
            }

            std::string op = op_v.get_string().c_str();
            response::position_direction pd = position_direction::UNDEFINED;

            if (op == "close")
            {
                //
                // Handle close and continue.
                //

                if (! operation_obj.contains("id"))
                {
                    throw response::violation_error("Missing ‘id’ in operation object");
                }

                value const &id_v = operation_obj.at("id");

                if (id_v.kind() != kind::string)
                {
                    throw response::violation_error("Invalid id type in operation object");
                }

                close_positions_.emplace_back(id_v.get_string().c_str());

                continue;
            }
            else if (op == "LONG")
            {
                pd = position_direction::POSITION_LONG;
            }
            else if (op == "SHORT")
            {
                pd = position_direction::POSITION_SHORT;
            }
            else
            {
                std::string err = std::string("Illegal operation ‘")
                                  + op + std::string("’ in operation object");

                throw response::violation_error(err);
            }

            //
            // Continue parsing ‘id’ and ‘qty’ attributes
            // for LONG or SHORT operation.
            //

            if (! operation_obj.contains("id"))
            {
                throw response::violation_error("Missing ‘id’ in operation object");
            }

            value const &id_v = operation_obj.at("id");

            if (id_v.kind() != kind::string)
            {
                throw response::violation_error("Invalid id type in operation object");
            }

            std::string id = id_v.get_string().c_str();

            if (! operation_obj.contains("qty"))
            {
                throw response::violation_error("Missing ‘qty’ in operation object");
            }

            value const &qty_v = operation_obj.at("qty");

            if (qty_v.kind() != kind::int64)
            {
                throw response::violation_error("Invalid qty type in operation object");
            }

            int qty = qty_v.get_int64();

            new_positions_.emplace_back(pd, id, qty);
        }
    }
    else
    {
        std::string err = std::string("Illegal status ‘")
                          + status + std::string("’");

        throw response::violation_error(err);
    }
}

} /* namespace protocol */
} /* namespace hft */

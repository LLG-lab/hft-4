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

#include <boost/json.hpp>

#include <hft_request.hpp>

namespace hft {
namespace protocol {
namespace request {

static init make_init(boost::json::object const &obj)
{
    using namespace boost::json;

    init ret;

    if (! obj.contains("sessid"))
    {
        ret.sessid = "";
    }
    else
    {
        value const &v = obj.at("sessid");

        if (v.kind() != kind::string)
        {
            throw violation_error("Invalid sessid attribute type for method init");
        }

        ret.sessid = v.get_string().c_str();
    }

    if (! obj.contains("instruments"))
    {
        throw violation_error("Missing instruments for method init");
    }

    value const &v = obj.at("instruments");

    if (v.kind() != kind::array)
    {
        throw violation_error("Invalid instruments attribute type for method init");
    }

    array const &arr = v.get_array();

    if (arr.empty())
    {
        throw violation_error("Method init requires at least one instrument");
    }

    for (auto it = arr.begin(); it != arr.end(); it++)
    {
        value const &array_val = *it;

        if (array_val.kind() != kind::string)
        {
            throw violation_error("Instrument ticker must be a string type for method init");
        }

        ret.instruments.push_back(std::string(array_val.get_string().c_str())); // XXX mozeby emplace zrobic?
    }

    return ret;
}

static sync make_sync(boost::json::object const &obj)
{
    using namespace boost::json;

    sync ret;

    //
    // Obtain instrument.
    //

    if (! obj.contains("instrument"))
    {
        throw violation_error("Missing instrument attribute for method sync");
    }

    value const &v_instrument = obj.at("instrument");

    if (v_instrument.kind() != kind::string)
    {
        throw violation_error("Invalid instrument attribute type for method sync");
    }

    ret.instrument = v_instrument.get_string().c_str();

    //
    // Obtain position ID.
    //

    if (! obj.contains("id"))
    {
        throw violation_error("Missing id attribute for method sync");
    }

    value const &v_id = obj.at("id");

    if (v_id.kind() != kind::string)
    {
        throw violation_error("Invalid id attribute type for method sync");
    }

    ret.id = v_id.get_string().c_str();

    //
    // Obtain created on.
    //

    if (! obj.contains("timestamp"))
    {
        throw violation_error("Missing timestamp attribute for method sync");
    }

    value const &v_timestamp = obj.at("timestamp");

    if (v_timestamp.kind() != kind::string)
    {
        throw violation_error("Invalid timestamp attribute type for method sync");
    }

    try
    {
        ret.created_on = boost::posix_time::ptime(boost::posix_time::time_from_string(v_timestamp.get_string().c_str()));
    }
    catch (const std::exception &e)
    {
        throw violation_error("Invalid format of timestamp attribute for method sync");
    }

    if (ret.created_on.is_not_a_date_time())
    {
        throw violation_error("Invalid format of timestamp attribute for method sync");
    }

    //
    // Obtain position direction.
    //

    if (! obj.contains("direction"))
    {
        throw violation_error("Missing direction attribute for method sync");
    }

    value const &v_direction = obj.at("direction");

    if (v_direction.kind() != kind::string)
    {
        throw violation_error("Invalid direction attribute type for method sync");
    }

    std::string direction = v_direction.get_string().c_str();

    if (direction == "LONG")
    {
        ret.is_long = true;
    }
    else if (direction == "SHORT")
    {
        ret.is_long = false;
    }
    else
    {
        throw violation_error("Illegal value of direction attribute for method sync");
    }

    //
    // Obtain price.
    //

    if (! obj.contains("price"))
    {
        throw violation_error("Missing price attribute for method sync");
    }

    value const &v_price = obj.at("price");

    if (v_price.kind() != kind::double_)
    {
        throw violation_error("Invalid price attribute type for method sync");
    }

    ret.price = v_price.get_double();

    //
    // Obtain contract quantity.
    //

    if (! obj.contains("qty"))
    {
        throw violation_error("Missing qty attribute for method sync");
    }

    value const &v_qty = obj.at("qty");

    if (v_qty.kind() == kind::uint64)
    {
        ret.qty = v_qty.get_uint64();
    }
    else if (v_qty.kind() == kind::int64)
    {
        ret.qty = v_qty.get_int64();
    }
    else
    {
        throw violation_error("Invalid qty attribute type for method sync");
    }

    return ret;
}

static tick make_tick(boost::json::object const &obj)
{
    using namespace boost::json;

    tick ret;

    //
    // Obtain instrument.
    //

    if (! obj.contains("instrument"))
    {
        throw violation_error("Missing instrument attribute for method tick");
    }

    value const &v_instrument = obj.at("instrument");

    if (v_instrument.kind() != kind::string)
    {
        throw violation_error("Invalid instrument attribute type for method tick");
    }

    ret.instrument = v_instrument.get_string().c_str();

    //
    // Obtain request time.
    //

    if (! obj.contains("timestamp"))
    {
        throw violation_error("Missing timestamp attribute for method tick");
    }

    value const &v_timestamp = obj.at("timestamp");

    if (v_timestamp.kind() != kind::string)
    {
        throw violation_error("Invalid timestamp attribute type for method tick");
    }

    try
    {
        ret.request_time = boost::posix_time::ptime(boost::posix_time::time_from_string(v_timestamp.get_string().c_str()));
    }
    catch (const std::exception &e)
    {
        throw violation_error("Invalid format of timestamp attribute for method tick");
    }

    if (ret.request_time.is_not_a_date_time())
    {
        throw violation_error("Invalid format of timestamp attribute for method tick");
    }

    //
    // Obtain ask.
    //

    if (! obj.contains("ask"))
    {
        throw violation_error("Missing ask attribute for method tick");
    }

    value const &v_ask = obj.at("ask");

    if (v_ask.kind() == kind::double_)
    {
        ret.ask = v_ask.get_double();
    }
    else if (v_ask.kind() == kind::int64)
    {
        ret.ask = v_ask.get_int64();
    }
    else if (v_ask.kind() == kind::uint64)
    {
        ret.ask = v_ask.get_uint64();
    }
    else
    {
        throw violation_error("Invalid ask attribute type for method tick");
    }



    //
    // Obtain bid.
    //

    if (! obj.contains("bid"))
    {
        throw violation_error("Missing bid attribute for method tick");
    }

    value const &v_bid = obj.at("bid");

    if (v_bid.kind() == kind::double_)
    {
        ret.bid = v_bid.get_double();
    }
    else if (v_bid.kind() == kind::int64)
    {
        ret.bid = v_bid.get_int64();
    }
    else if (v_bid.kind() == kind::uint64)
    {
        ret.bid = v_bid.get_uint64();
    }
    else
    {
        throw violation_error("Invalid bid attribute type for method tick");
    }

    //
    // Obtain equity.
    //

    if (! obj.contains("equity"))
    {
        throw violation_error("Missing equity attribute for method tick");
    }

    value const &v_equity = obj.at("equity");

    if (v_equity.kind() == kind::double_)
    {
        ret.equity = v_equity.get_double();
    }
    else if (v_equity.kind() == kind::int64)
    {
        ret.equity = v_equity.get_int64();
    }
    else
    {
        throw violation_error("Invalid equity attribute type for method tick");
    }

    return ret;
}

static open_notify make_open_notify(boost::json::object const &obj)
{
    using namespace boost::json;

    open_notify ret;

    //
    // Obtain instrument.
    //

    if (! obj.contains("instrument"))
    {
        throw violation_error("Missing instrument attribute for method open_notify");
    }

    value const &v_instrument = obj.at("instrument");

    if (v_instrument.kind() != kind::string)
    {
        throw violation_error("Invalid instrument attribute type for method open_notify");
    }

    ret.instrument = v_instrument.get_string().c_str();

    //
    // Obtain position ID.
    //

    if (! obj.contains("id"))
    {
        throw violation_error("Missing id attribute for method open_notify");
    }

    value const &v_id = obj.at("id");

    if (v_id.kind() != kind::string)
    {
        throw violation_error("Invalid id attribute type for method open_notify");
    }

    ret.id = v_id.get_string().c_str();

    //
    // Obtain status.
    //

    if (! obj.contains("status"))
    {
        throw violation_error("Missing status attribute for method open_notify");
    }

    value const &v_status = obj.at("status");

    if (v_status.kind() != kind::bool_)
    {
        throw violation_error("Invalid status attribute type for method open_notify");
    }

    ret.status = v_status.get_bool();

    //
    // Obtain final transaction price.
    //

    if (! obj.contains("price"))
    {
        throw violation_error("Missing price attribute for method open_notify");
    }

    value const &v_price = obj.at("price");

    if (v_price.kind() == kind::double_)
    {
        ret.price = v_price.get_double();
    }
    else if (v_price.kind() == kind::int64)
    {
        ret.price = v_price.get_int64();
    }
    else if (v_price.kind() == kind::uint64)
    {
        ret.price = v_price.get_uint64();
    }
    else
    {
        throw violation_error("Invalid price attribute type for method open_notify");
    }

    return ret;
}

static close_notify make_close_notify(boost::json::object const &obj)
{
    using namespace boost::json;

    close_notify ret;
    //
    // Obtain instrument.
    //

    if (! obj.contains("instrument"))
    {
        throw violation_error("Missing instrument attribute for method close_notify");
    }

    value const &v_instrument = obj.at("instrument");

    if (v_instrument.kind() != kind::string)
    {
        throw violation_error("Invalid instrument attribute type for method close_notify");
    }

    ret.instrument = v_instrument.get_string().c_str();

    //
    // Obtain position ID.
    //

    if (! obj.contains("id"))
    {
        throw violation_error("Missing id attribute for method close_notify");
    }

    value const &v_id = obj.at("id");

    if (v_id.kind() != kind::string)
    {
        throw violation_error("Invalid id attribute type for method close_notify");
    }

    ret.id = v_id.get_string().c_str();

    //
    // Obtain status.
    //

    if (! obj.contains("status"))
    {
        throw violation_error("Missing status attribute for method close_notify");
    }

    value const &v_status = obj.at("status");

    if (v_status.kind() != kind::bool_)
    {
        throw violation_error("Invalid status attribute type for method close_notify");
    }

    ret.status = v_status.get_bool();

    //
    // Obtain final transaction price.
    //

    if (! obj.contains("price"))
    {
        throw violation_error("Missing price attribute for method close_notify");
    }

    value const &v_price = obj.at("price");

    if (v_price.kind() == kind::double_)
    {
        ret.price = v_price.get_double();
    }
    else if (v_price.kind() == kind::int64)
    {
        ret.price = v_price.get_int64();
    }
    else if (v_price.kind() == kind::uint64)
    {
        ret.price = v_price.get_uint64();
    }
    else
    {
        throw violation_error("Invalid price attribute type for method close_notify");
    }

    return ret;
}

} /* namespace request */

request::generic parse_request_payload(const std::string &json_data)
{
    using namespace boost::json;

    value jv;

    try
    {
        jv = parse(json_data);
    }
    catch (const system_error &e)
    {
        throw request::violation_error("JSON parse error");
    }

    if (jv.kind() == kind::object)
    {
        object const &obj = jv.get_object();

        if (obj.empty())
        {
            throw request::violation_error("Empty request");
        }

        if (! obj.contains("method"))
        {
            throw request::violation_error("Missing method");
        }

        value const &v = obj.at("method");

        if (v.kind() != kind::string)
        {
            throw request::violation_error("Invalid method type");
        }

        std::string method = v.get_string().c_str();

        //
        // The order of checking should depend on
        // the frequency of the request type.
        //

        if (method == "tick")
        {
            return request::make_tick(obj);
        }
        else if (method == "open_notify")
        {
            return request::make_open_notify(obj);
        }
        else if (method == "close_notify")
        {
            return request::make_close_notify(obj);
        }
        else if (method == "sync")
        {
            return request::make_sync(obj);
        }
        else if (method == "init")
        {
            return request::make_init(obj);
        }

        std::string error_message = std::string("Illegal method: ") + method;

        throw request::violation_error(error_message);
    }
    else
    {
        throw request::violation_error("Bad request");
    }
}

} /* namespace protocol */
} /* namespace hft */

/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System ® ≣≡=-              **
**                                                                    **
**          Copyright © 2017 - 2025 by LLG Ryszard Gradowski          **
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

#ifndef __HFT_REQUEST_HPP__
#define __HFT_REQUEST_HPP__

#include <stdexcept>
#include <vector>
#include <boost/variant2/variant.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <custom_except.hpp>

namespace hft {
namespace protocol {
namespace request {

DEFINE_CUSTOM_EXCEPTION_CLASS(violation_error, std::runtime_error)

struct init
{
    enum { OPCODE = 0 };

    std::string sessid;
    std::vector<std::string> instruments;
};

// {"method":"sync","instrument":"EUR/USD","id":"a87f6d","direction":"LONG","price":1.23459,"qty":1000}
struct sync
{
    enum { OPCODE = 1 };

    std::string instrument;
    std::string id;
    boost::posix_time::ptime created_on;
    bool is_long;
    double price;
    int qty;
};

// {"method":"tick","instrument":"EUR/USD","timestamp":"2019-10-26 20:45:31.000","ask":1.3145,"bid":1.2456,"equity":56432}
struct tick
{
    enum { OPCODE = 2 };

    std::string instrument;
    boost::posix_time::ptime request_time;
    double ask;
    double bid;
    double equity;
    double free_margin;
};

// {"method":"open_notify","instrument":"EUR/USD","id":"ahd76s","status":false,"price":1.23}
struct open_notify
{
    enum { OPCODE = 3 };

    std::string instrument;
    std::string id;
    double price;
    bool status;
};

// {"method":"close_notify","instrument":"EUR/USD","id":"ahd76s","status":false}
struct close_notify
{
    enum { OPCODE = 4 };

    std::string instrument;
    std::string id;
    double price;
    bool status;
};

typedef boost::variant2::variant<init,
                                 sync,
                                 tick,
                                 open_notify,
                                 close_notify> generic; 

} /* namespace request */

request::generic parse_request_payload(const std::string &json_data);

} /* namespace protocol */
} /* namespace hft */

#endif /* __HFT_REQUEST_HPP__ */

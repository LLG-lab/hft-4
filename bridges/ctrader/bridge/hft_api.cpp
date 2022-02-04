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

#include <hft_api.hpp>
#include <aux_functions.hpp>

#include <sstream>
#include <stdexcept>

void hft_api::hft_init_session(const std::string &sessid, const instruments_container &instruments)
{
    if (instruments.empty())
    {
        throw std::runtime_error("hft_api: hft_init_session: Empty instrument set");
    }

    if (sessid.empty())
    {
        throw std::runtime_error("hft_api: hft_init_session: Empty session id");
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
        throw std::runtime_error("hft_api: hft_sync: Empty instrument");
    }

    if (identifier.empty())
    {
        throw std::runtime_error("hft_api: hft_sync: Empty position identifier");
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
            throw std::runtime_error("hft_api: hft_sync: Illegal trade side");
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
        throw std::runtime_error("hft_api: hft_send_tick: Empty instrument");
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
        throw std::runtime_error("hft_api: hft_send_open_notify: Empty instrument");
    }

    if (identifier.empty())
    {
        throw std::runtime_error("hft_api: hft_send_open_notify: Empty position identifier");
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
        throw std::runtime_error("hft_api: hft_send_close_notify: Empty instrument");
    }

    if (identifier.empty())
    {
        throw std::runtime_error("hft_api: hft_send_close_notify: Empty position identifier");
    }

    std::ostringstream payload;

    std::string s = (status ? "true" : "false");

    payload << "{\"method\":\"close_notify\",\"instrument\":\""
            << instrument << "\",\"id\":\"" << identifier
            << "\",\"status\":" << s << ",\"price\":"
            << price << "}\n";

    connection_.send_data(payload.str());
}

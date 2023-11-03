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

#include <sstream>

#include <hft_server_connector.hpp>

hft_server_connector::hft_server_connector(const std::string &host, const std::string &port)
    : ioctx_(), socket_(ioctx_)
{
    boost::asio::ip::tcp::resolver resolver(ioctx_);
    boost::asio::connect(socket_, resolver.resolve({host, port}));
}

hft_server_connector::~hft_server_connector(void)
{
}

void hft_server_connector::init(const std::string &sessid, const std::vector<std::string> &instruments)
{
    if (instruments.empty())
    {
        throw std::runtime_error("hft_server_connector: init: Empty instrument set");
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

    hft::protocol::response rsp;
    rsp.unserialize(send_recv_server(payload.str()));

    if (rsp.is_error())
    {
        throw std::runtime_error(rsp.get_error_message());
    }
}

void hft_server_connector::send_tick(const std::string &instrument, double balance, double free_margin,
                                         const csv_data_supplier::csv_record &tick_info, hft::protocol::response &rsp)
{
    std::ostringstream payload;

    payload << "{\"method\":\"tick\",\"instrument\":\""
            << instrument << "\",\"timestamp\":\""
            << tick_info.request_time << "\",\"ask\":"
            << tick_info.ask << ",\"bid\":"
            << tick_info.bid << ",\"equity\":"
            << balance  << ",\"free_margin\":"
            << free_margin << "}\n";

    rsp.unserialize(send_recv_server(payload.str()));
}

void hft_server_connector::send_open_notify(const std::string &instrument, const std::string &position_id,
                                                bool status, double price, hft::protocol::response &rsp)
{
    std::ostringstream payload;

    std::string s = (status ? "true" : "false");

    payload << "{\"method\":\"open_notify\",\"instrument\":\""
            << instrument << "\",\"id\":\"" << position_id
            << "\",\"status\":" << s << ",\"price\":"
            << price << "}\n";

    rsp.unserialize(send_recv_server(payload.str()));
}

void hft_server_connector::send_close_notify(const std::string &instrument, const std::string &position_id,
                                                 bool status, double price, hft::protocol::response &rsp)
{
    std::ostringstream payload;

    std::string s = (status ? "true" : "false");

    payload << "{\"method\":\"close_notify\",\"instrument\":\""
            << instrument << "\",\"id\":\"" << position_id
            << "\",\"status\":" << s << ",\"price\":"
            << price << "}\n";

    rsp.unserialize(send_recv_server(payload.str()));
}

std::string hft_server_connector::send_recv_server(const std::string &payload)
{
    std::string reply;

    boost::asio::write(socket_, boost::asio::buffer(payload.c_str(), payload.length()));

    boost::asio::streambuf data_from_server;
    size_t reply_length = boost::asio::read_until(socket_, data_from_server, '\n');
    std::istream str(&data_from_server);
    std::getline(str, reply);

    return reply;
}

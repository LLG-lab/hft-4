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

#include <sstream>

#include <hft_server_connector.hpp>

hft_server_connector::hft_server_connector(const std::string &host, const std::string &port,
                                               const std::string &instrument, const std::string &sessid)
    : ioctx_(), socket_(ioctx_), instrument_(instrument)
{

    boost::asio::ip::tcp::resolver resolver(ioctx_);
    boost::asio::connect(socket_, resolver.resolve({host, port}));

    //
    // Sending ‘init’ to HFT server.
    //

    std::ostringstream payload;

    payload << "{\"method\":\"init\",\"sessid\":\""
            << sessid << "\",\"instruments\":[\""
            << instrument << "\"]}\n";
    
    hft::protocol::response rsp;
    rsp.unserialize(send_recv_server(payload.str()));

    if (rsp.is_error())
    {
        throw std::runtime_error(rsp.get_error_message());
    }
}

hft_server_connector::~hft_server_connector(void)
{
}

void hft_server_connector::send_tick(double equity, const csv_data_supplier::csv_record &tick_record,
                                         hft::protocol::response &rsp)
{
    std::ostringstream payload;

    payload << "{\"method\":\"tick\",\"instrument\":\""
            << instrument_ << "\",\"timestamp\":\""
            << tick_record.request_time << "\",\"ask\":"
            << tick_record.ask << ",\"bid\":"
            << tick_record.bid << ",\"equity\":"
            << equity << "}\n";

    rsp.unserialize(send_recv_server(payload.str()));
}

void hft_server_connector::send_open_notify(const std::string &position_id, bool status,
                                                double price, hft::protocol::response &rsp)
{
    std::ostringstream payload;

    std::string s = (status ? "true" : "false");

    payload << "{\"method\":\"open_notify\",\"instrument\":\""
            << instrument_ << "\",\"id\":\"" << position_id
            << "\",\"status\":" << s << ",\"price\":"
            << price << "}\n";

    rsp.unserialize(send_recv_server(payload.str()));
}

void hft_server_connector::send_close_notify(const std::string &position_id, bool status,
                                                 double price, hft::protocol::response &rsp)
{
    std::ostringstream payload;

    std::string s = (status ? "true" : "false");

    payload << "{\"method\":\"close_notify\",\"instrument\":\""
            << instrument_ << "\",\"id\":\"" << position_id
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

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

#ifndef __HFT_SERVER_CONNECTION_HPP__
#define __HFT_SERVER_CONNECTION_HPP__

#include <boost/asio.hpp>

#include <hft_response.hpp>
#include <csv_data_supplier.hpp>

class hft_server_connector : private boost::noncopyable
{
public:

    hft_server_connector(void) = delete;

    hft_server_connector(const std::string &host, const std::string &port);

    ~hft_server_connector(void);

    void init(const std::string &sessid, const std::vector<std::string> &instruments);

    void send_tick(const std::string &instrument, double balance, double free_margin,
                       const csv_data_supplier::csv_record &tick_info, hft::protocol::response &rsp);

    void send_open_notify(const std::string &instrument, const std::string &position_id,
                              bool status, double price, hft::protocol::response &rsp);

    void send_close_notify(const std::string &instrument, const std::string &position_id,
                               bool status, double price, hft::protocol::response &rsp);

private:

    std::string send_recv_server(const std::string &payload);

    boost::asio::io_context ioctx_;
    boost::asio::ip::tcp::socket socket_;

};

#endif /* __HFT_SERVER_CONNECTION_HPP__ */

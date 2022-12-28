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

#ifndef __HFT_CONNECTION_HPP__
#define __HFT_CONNECTION_HPP__

#include <boost/asio.hpp>
//#include <boost/asio/steady_timer.hpp>
#include <functional>
#include <string>
#include <list>

#include <hft2ctrader_config.hpp>

class hft_connection
{
public:

    hft_connection(void) = delete;
    hft_connection(hft_connection &) = delete;
    hft_connection(hft_connection &&) = delete;

    hft_connection(boost::asio::io_context &io_context, const hft2ctrader_config &cfg);

    ~hft_connection(void);

    void connect(void);

    void close(void);

    void send_data(const std::string &data);

    //
    // Configurable callbacks.
    //

    void set_on_data_callback(const std::function<void(const std::string &)> &callback) { on_data = callback; }

private:

    std::function<void(const std::string &)> on_data;

    void async_read_raw_message(void);
    void async_write_raw_message(void);
    void try_reconnect_after_a_while(void);

    boost::asio::ip::tcp::socket socket_;
    boost::asio::steady_timer reconnect_timer_;

    std::string input_buffer_;
    std::list<std::string> send_buffers_;

    std::string hft_host_;
    std::string hft_port_;

    bool connected_;
    int  connection_attempts_;

    boost::asio::io_context &ioctx_;
};

#endif /*  __HFT_CONNECTION_HPP__ */

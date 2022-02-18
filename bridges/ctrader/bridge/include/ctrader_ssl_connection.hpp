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

#ifndef __CTRADER_SSL_CONNECTION_HPP__
#define __CTRADER_SSL_CONNECTION_HPP__

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <functional>
#include <vector>
#include <list>

#include <hft2ctrader_config.hpp>

class ctrader_ssl_connection
{
public:

    ctrader_ssl_connection(void) = delete;
    ctrader_ssl_connection(ctrader_ssl_connection &) = delete;
    ctrader_ssl_connection(ctrader_ssl_connection &&) = delete;

    ctrader_ssl_connection(boost::asio::io_context &io_context, const hft2ctrader_config &cfg);

    ~ctrader_ssl_connection(void);

    void connect(void);

    void close(void);

    void send_data(const std::vector<char> &data);

    //
    // Configurable callbacks.
    //

    void set_on_error_callback(const std::function<void(const boost::system::error_code &)> &callback) { on_error = callback; }
    void set_on_data_callback(const std::function<void(const std::vector<char> &)> &callback) { on_data = callback; }
    void set_on_connected_callback(const std::function<void(void)> &callback) { on_connected = callback; }

private:

    enum class connection_stage_type
    {
        IDLE,
        CONNECTING,
        CONNECTED
    };

    void handshake(void);
    void async_read_raw_message(void);
    void async_read_msg_chunk(void);
    void transmit(void);

    void try_reconnect_after_a_while(void);

    std::string ctrader_port_;
    std::string ctrader_host_;

    int  connection_attempts_;

    boost::asio::ssl::context ssl_ctx_;
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket_;
    boost::asio::steady_timer reconnect_timer_;
    boost::asio::io_context &ioctx_;

    std::function<void(const boost::system::error_code &)> on_error;
    std::function<void(const std::vector<char> &)> on_data;
    std::function<void(void)> on_connected;

    char message_len_buf_[4];
    int message_len_;
    int message_total_read_;
    std::vector<char> message_buf_;
    std::list<std::vector<char>> send_buffers_;

    connection_stage_type connection_stage_;
};

#endif /* __CTRADER_SSL_CONNECTION_HPP__ */

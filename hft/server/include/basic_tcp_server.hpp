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

#ifndef __BASIC_TCP_SERVER_HPP__
#define __BASIC_TCP_SERVER_HPP__

#include <iostream>

#include <boost/bind.hpp>
#include <boost/asio.hpp>

#include <easylogging++.h>

using boost::asio::ip::tcp;

#define hft_tcpserver_log(__X__) \
    CLOG(__X__, "TCP_IP_server")

template <typename SessionType>
class basic_tcp_server
{
public:

    basic_tcp_server(boost::asio::io_context &ioctx, const std::string &endpoint, short port)
        : ioctx_(ioctx),
          acceptor_(ioctx, tcp::endpoint(boost::asio::ip::address::from_string(endpoint), port))
    {
        //
        // Creates logger with id `TCP_IP_server'.
        //

        el::Loggers::getLogger("TCP_IP_server", true);

        hft_tcpserver_log(INFO) << "Server configured to listen "
                                << "on port [" << port << "]";

        start_accept();
    }

    virtual ~basic_tcp_server(void) = default;

private:

    void start_accept(void)
    {
        SessionType *new_session = new SessionType(ioctx_);

        acceptor_.async_accept(new_session -> socket(), boost::bind(&basic_tcp_server<SessionType>::handle_accept, this, new_session, boost::asio::placeholders::error));
    }

    void handle_accept(SessionType *new_session, const boost::system::error_code &error)
    {
        if (! error)
        {
            new_session -> start();
        }
        else
        {
            hft_tcpserver_log(ERROR) << "Terminate session because of system error: "
                                     << error.message();

            delete new_session;
        }

        start_accept();
    }

    boost::asio::io_context &ioctx_;
    tcp::acceptor acceptor_;
};


#endif /* __BASIC_TCP_SERVER_HPP__ */

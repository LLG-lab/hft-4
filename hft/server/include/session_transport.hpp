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

#ifndef __SESSION_TRANSPORT_HPP__
#define __SESSION_TRANSPORT_HPP__

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <iostream>

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <easylogging++.h>

#include <deallocator.hpp>
#include <hft_request.hpp>

using boost::asio::ip::tcp;

class session_transport : public deallocator
{
public:

    session_transport(boost::asio::io_service &io_service)
        : socket_(io_service), request_time_(0,0,0,0)
    {
         el::Loggers::getLogger("transport", true);
    }

    virtual ~session_transport(void) = default;

    tcp::socket &socket(void)
    {
        return socket_;
    }

    void start(void)
    {
        boost::asio::async_read_until(socket_, input_buffer_, '\n', boost::bind(&session_transport::handle_read, this, _1));
    }

protected:

    const boost::posix_time::time_duration &get_request_time(void) const
    {
        return request_time_;
    }

    //
    // Methods to be implemented in hft_session class.
    //

    virtual void handle_init_request(const hft::protocol::request::init &msg, std::string &response_payload) = 0;
    virtual void handle_sync_request(const hft::protocol::request::sync &msg, std::string &response_payload) = 0;
    virtual void handle_tick_request(const hft::protocol::request::tick &msg, std::string &response_payload) = 0;
    virtual void handle_open_notify_request(const hft::protocol::request::open_notify &msg, std::string &response_payload) = 0;
    virtual void handle_close_notify_request(const hft::protocol::request::close_notify &msg, std::string &response_payload) = 0;

private:

    void handle_read(const boost::system::error_code &error);

    void handle_write(const boost::system::error_code &error);

    tcp::socket socket_;
    boost::asio::streambuf input_buffer_;
    std::string response_data_;

    boost::posix_time::time_duration request_time_;
};

#endif /* __SESSION_TRANSPORT_HPP__ */

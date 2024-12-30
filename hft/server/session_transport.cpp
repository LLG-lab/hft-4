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

#include <ctime>
#include <cstdio>

#include <session_transport.hpp>
#include <hft_response.hpp>

#define hft_log(__X__) \
    CLOG(__X__, "transport")

void session_transport::handle_read(const boost::system::error_code &error)
{
    if (! error)
    {
        //
        // Extract the newline-delimited message from the buffer.
        //

        std::string line;
        std::istream is(&input_buffer_);
        std::getline(is, line);

        try
        {
            hft::protocol::request::generic msg = hft::protocol::parse_request_payload(line);

            switch (msg.index())
            {
                case hft::protocol::request::init::OPCODE:
                     handle_init_request(boost::variant2::get<hft::protocol::request::init::OPCODE>(msg), response_data_);
                     break;
                case hft::protocol::request::sync::OPCODE:
                     handle_sync_request(boost::variant2::get<hft::protocol::request::sync::OPCODE>(msg), response_data_);
                     break;
                case hft::protocol::request::tick::OPCODE:
                     handle_tick_request(boost::variant2::get<hft::protocol::request::tick::OPCODE>(msg), response_data_);
                     break;
                case hft::protocol::request::open_notify::OPCODE:
                     handle_open_notify_request(boost::variant2::get<hft::protocol::request::open_notify::OPCODE>(msg), response_data_);
                     break;
                case hft::protocol::request::close_notify::OPCODE:
                     handle_close_notify_request(boost::variant2::get<hft::protocol::request::close_notify::OPCODE>(msg), response_data_);
                     break;
                default:
                    throw std::runtime_error("Transport error");
            }
        }
        catch (const hft::protocol::request::violation_error &e)
        {
            hft_log(ERROR) << "Protocol violation error: " << e.what()
                           << ". Client request: ‘" << line << "’";


            hft::protocol::response resp;
            resp.error(e.what());
            response_data_ = resp.serialize();
        }
        catch (const std::exception &e)
        {
            hft_log(ERROR) << "Error occured: " << e.what()
                           << ". Going to close the session";

            //  Wcześniej było źle raczej: socket_.close();
            delete this;

            return;
        }

        boost::asio::async_write(socket_,
                                 boost::asio::buffer(response_data_.c_str(), response_data_.length()),
                                 boost::bind(&session_transport::handle_write, this, boost::asio::placeholders::error));
    }
    else
    {
        hft_log(WARNING) << "Destroying session because of error ("
                         << error.value() << "): " << error.message();

        delete this;
    }
}

void session_transport::handle_write(const boost::system::error_code& error)
{
    if (! error)
    {
        boost::asio::async_read_until(socket_, input_buffer_, '\n', boost::bind(&session_transport::handle_read, this, _1));
    }
    else
    {
        hft_log(WARNING) << "Destroying session because of error ("
                         << error.value() << "): " << error.message();

        delete this;
    }
}

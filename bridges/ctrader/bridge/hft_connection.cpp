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

#include <hft_connection.hpp>

#include <easylogging++.h>

#define hft2ctrader_log(__X__) \
    CLOG(__X__, "hft_connection")

hft_connection::hft_connection(boost::asio::io_context &io_context, const hft2ctrader_config &cfg)
    : socket_ {io_context}, reconnect_timer_ {io_context}, input_buffer_ {}, send_buffers_ {},
      hft_host_ { cfg.get_hft_host() }, hft_port_ { std::to_string(cfg.get_hft_port())},
      connected_ { false }, connection_attempts_{0}, ioctx_ {io_context}
{
    el::Loggers::getLogger("hft_connection", true);
}

hft_connection::~hft_connection(void)
{
    close();
}


void hft_connection::connect(void)
{
    if (! on_data)
    {
        hft2ctrader_log(ERROR) << "Undefined ‘on_data’ callback";

        throw std::logic_error("Undefined ‘on_data’ callback");
    }

    boost::asio::ip::tcp::resolver resolver(ioctx_);

    auto resolve_result = resolver.resolve({hft_host_, hft_port_});
    auto x = resolve_result.begin();

    if (x == resolve_result.end())
    {
        hft2ctrader_log(ERROR) << "No available HFT endpoint";

        throw std::logic_error("No available HFT endpoint");
    }

    auto endpoint = x -> endpoint();

    //
    // Start connection.
    //

    hft2ctrader_log(INFO) << "Starting async connection to the HFT server "
                          << (*resolve_result.begin()).host_name() << " ("
                          << (*resolve_result.begin()).endpoint().address().to_string()
                          << ")";

    socket_.async_connect(endpoint,
        [this](const boost::system::error_code& error)
        {
            if (error)
            {
                connected_ = false;

                hft2ctrader_log(ERROR) << "Connection to the HFT server "
                                       << " failed ("
                                       << error.value() << "): "
                                       << error.message() << ".";

                try_reconnect_after_a_while();
            }
            else
            {
                hft2ctrader_log(INFO) << "HFT connection ESTABLISHED.";

                connection_attempts_ = 0;
                connected_ = true;

                async_read_raw_message();
                async_write_raw_message();
            }           
        }
    );
}

void hft_connection::close(void)
{
    reconnect_timer_.cancel();
    socket_.close();
}

void hft_connection::send_data(const std::string &data)
{
    bool can_transmit = connected_ && (send_buffers_.size() == 0);

    send_buffers_.push_back(data);

    if (can_transmit)
    {
        async_write_raw_message();
    }
}

void hft_connection::async_read_raw_message(void)
{
    //
    // Start an asynchronous operation
    // to read a newline-delimited
    // message.
    //

    boost::asio::async_read_until(socket_, boost::asio::dynamic_buffer(input_buffer_), '\n',
        [this](const boost::system::error_code& ec, std::size_t n)
        {
            if (! ec)
            {
                //
                // Extract the newline-delimited message from the buffer.
                //

                std::string line(input_buffer_.substr(0, n - 1));
                input_buffer_.erase(0, n);

                //
                // Empty messages are ignored.
                //

                if (! line.empty())
                {
                    on_data(line);
                }

                async_read_raw_message();
            }
            else
            {
                hft2ctrader_log(ERROR) << "HFT Read data error ("
                                       << ec.value() << "): "
                                       << ec.message() << ".";

                throw std::runtime_error("HFT Read data error");
            }
        }
    );
}

void hft_connection::async_write_raw_message(void)
{
    if (send_buffers_.size() == 0)
    {
        return;
    }

    boost::asio::async_write(socket_, boost::asio::buffer(&send_buffers_.begin() -> front(), send_buffers_.begin() -> length()),
        [this](const boost::system::error_code &error, std::size_t length)
        {
            if (! error)
            {
                send_buffers_.pop_front();

                async_write_raw_message();
            }
            else
            {
                send_buffers_.clear();

                hft2ctrader_log(ERROR) << "HFT Write data error ("
                                       << error.value() << "): "
                                       << error.message() << ".";

                throw std::runtime_error("HFT Write data error");
            }
        });
}

void hft_connection::try_reconnect_after_a_while(void)
{
    if (connection_attempts_ >= 10)
    {
        hft2ctrader_log(ERROR) << "I can't connect to the HFT server, tried 10 times and give up";

        throw std::runtime_error("HFT server connection error");
    }
    else
    {
        connection_attempts_++;

        reconnect_timer_.expires_after(boost::asio::chrono::seconds(1));
        reconnect_timer_.async_wait(
            [this](const boost::system::error_code &error)
            {
                if (! error)
                {
                    hft2ctrader_log(INFO) << "Attempt to connect to HFT, trial #"
                                          << (connection_attempts_ + 1);

                    connect();
                }
                else if (error == boost::asio::error::operation_aborted)
                {
                    hft2ctrader_log(WARNING) << "Attempt to connect to HFT aborted!";
                }
            }
        );
    }
}

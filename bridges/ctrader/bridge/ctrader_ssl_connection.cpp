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

#include <ctrader_ssl_connection.hpp>

#include <sstream>
#include <cstring>

#include <easylogging++.h>

#define hft2ctrader_log(__X__) \
    CLOG(__X__, "ssl_connection")

ctrader_ssl_connection::ctrader_ssl_connection(boost::asio::io_context& io_context, const hft2ctrader_config &cfg)
    : ctrader_port_ { "5035" },
      ctrader_host_ { "" },
      connection_attempts_ {0},
      ssl_ctx_ { boost::asio::ssl::context::sslv23 },
      socket_ {io_context, ssl_ctx_ },
      reconnect_timer_ {io_context},
      ioctx_ {io_context},
      connection_stage_ { connection_stage_type::IDLE }
{
    switch (cfg.get_account_type())
    {
        case hft2ctrader_config::account_type::DEMO_ACCOUNT:
             ctrader_host_ = "demo.ctraderapi.com";
             break;
        case hft2ctrader_config::account_type::LIVE_ACCOUNT:
             ctrader_host_ = "live.ctraderapi.com";
             break;
        default:
             throw std::logic_error("Illegal broker account type");
    }

    el::Loggers::getLogger("ssl_connection", true);
}

ctrader_ssl_connection::~ctrader_ssl_connection(void)
{
    close();
}

void ctrader_ssl_connection::connect(void)
{
    //
    // Check callbacks.
    //

    if (! on_connected)
    {
        hft2ctrader_log(ERROR) << "Undefined ‘on_connected’ callback";

        throw std::logic_error("Undefined ‘on_connected’ callback");
    }

    if (! on_error)
    {
        hft2ctrader_log(ERROR) << "Undefined ‘on_error’ callback";

        throw std::logic_error("Undefined ‘on_error’ callback");
    }

    if (! on_data)
    {
        hft2ctrader_log(ERROR) << "Undefined ‘on_data’ callback";

        throw std::logic_error("Undefined ‘on_data’ callback");
    }

    //
    // Check connection stage.
    //

    switch (connection_stage_)
    {
        case connection_stage_type::CONNECTING:
            return;
        case connection_stage_type::CONNECTED:
            hft2ctrader_log(WARNING) << "Server already connected";
            return;
        case connection_stage_type::IDLE:
            connection_stage_ = connection_stage_type::CONNECTING;
            break;
    }

    //
    // Start connection.
    //

    boost::asio::ip::tcp::resolver resolver(ioctx_);
    auto endpoint = resolver.resolve(ctrader_host_, ctrader_port_);

    socket_.set_verify_mode(boost::asio::ssl::verify_peer);
    socket_.set_verify_callback([this](auto preverified, auto &ctx) {
            // FIXME: Potem rozkminić jak to obsłużyć pożądnie.
            char subject_name[256];
            X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
            X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
            hft2ctrader_log(INFO) << "Verifying " << subject_name;

			return true; // Jeśli cert ok, ma zwrócić true.
	});

    hft2ctrader_log(INFO) << "Starting async connection to the server "
                          << (*endpoint.begin()).host_name() << " ("
                          << (*endpoint.begin()).endpoint().address().to_string()
                          << ")";

    boost::asio::async_connect(socket_.lowest_layer(), endpoint,
        [this](const boost::system::error_code &error, const boost::asio::ip::tcp::endpoint &endpoint)
        {
            if (! error)
            {
                connection_attempts_ = 0;

                handshake();
            }
            else
            {
                hft2ctrader_log(ERROR) << "Connection to the server "
                                       << endpoint.address().to_string()
                                       << " failed ("
                                       << error.value() << "): "
                                       << error.message() << ".";

                try_reconnect_after_a_while();
            }
        });
}

void ctrader_ssl_connection::close(void)
{
    boost::system::error_code ec;

    reconnect_timer_.cancel();

    socket_.shutdown(ec);

    if (ec)
    {
        hft2ctrader_log(ERROR) << "Shutdown connection error ("
                               << ec.value() << "): "
                               << ec.message() << ".";
    }

    socket_.lowest_layer().close(ec);

    if (ec)
    {
        hft2ctrader_log(ERROR) << "Close connection error ("
                               << ec.value() << "): "
                               << ec.message() << ".";
    }

    hft2ctrader_log(INFO) << "Connection to cTrader closed.";

    connection_stage_ = connection_stage_type::IDLE;
}

void ctrader_ssl_connection::send_data(const std::vector<char> &data)
{
    bool can_transmit = (send_buffers_.size() == 0);

    send_buffers_.emplace_back(data.size() + 4);
    auto last_item_it = send_buffers_.end(); --last_item_it;
    int data_size_big_endian = htonl(data.size());
    memcpy(&last_item_it -> front(), &data_size_big_endian, 4);
    memcpy(&(last_item_it -> front()) + 4, &data.front(), data.size());

    if (can_transmit)
    {
        transmit();
    }
}

void ctrader_ssl_connection::handshake(void)
{
    hft2ctrader_log(INFO) << "Starting async SSL/TLS negotiation.";

    socket_.async_handshake(boost::asio::ssl::stream_base::client,
        [this](const boost::system::error_code &error)
        {
            if (! error)
            {
                connection_stage_ = connection_stage_type::CONNECTED;

                async_read_raw_message();

                on_connected();
            }
            else
            {
                std::ostringstream err_msg;

                //
                // FIXME: Tu by możnabyło w zależności czy błąd
                //        to zerwanie połączenia, spróbować nawiązać
                //        połaczenie ponownie. Fatala rzucić przy każdym
                //        innym błędzie niż zerwanie połączenia.
                //

                err_msg << "Handshake failed (" << error.value() << "): "
                        << error.message() << ".";

                hft2ctrader_log(ERROR) << err_msg.str();

                throw std::runtime_error(err_msg.str());
            }
        });
}

void ctrader_ssl_connection::async_read_raw_message(void)
{
    message_len_ = 0;

    boost::asio::async_read(socket_, boost::asio::buffer(message_len_buf_, 4),
        [this](const boost::system::error_code &error, std::size_t length)
        {
            if (! error)
            {
                if (length != 4)
                {
                    // FIXME: Dorobić obsługę. Sworzyć jakiś
                    //        error code na to albo wykorzystać
                    //        istniejący. Wywołać on_error.
                }

                memcpy(&message_len_, message_len_buf_, 4);
                message_len_ = ntohl(message_len_);
                message_buf_.clear();
                message_buf_.resize(message_len_);
                message_total_read_ = 0;

                async_read_msg_chunk();
            }
            else
            {
                hft2ctrader_log(ERROR) << "SSL Read data error ("
                                       << error.value() << "): "
                                       << error.message() << ".";

                if (connection_stage_ != connection_stage_type::CONNECTING)
                {
                    on_error(error);
                }
            }
        });
}

void ctrader_ssl_connection::async_read_msg_chunk(void)
{
    boost::asio::async_read(socket_, boost::asio::buffer(&message_buf_.front()+message_total_read_, message_len_-message_total_read_),
        [this](const boost::system::error_code &error, std::size_t length)
        {
            if (! error)
            {
                message_total_read_ += length;

                if (message_len_ == message_total_read_)
                {
                    on_data(message_buf_);

                    async_read_raw_message();
                }
                else
                {
                    async_read_msg_chunk();
                }
            }
            else
            {
                hft2ctrader_log(ERROR) << "SSL Read data error ("
                                       << error.value() << "): "
                                       << error.message() << ".";

                if (connection_stage_ != connection_stage_type::CONNECTING)
                {
                    on_error(error);
                }
            }
        });
}

void ctrader_ssl_connection::transmit(void)
{
    boost::asio::async_write(socket_, boost::asio::buffer(&send_buffers_.begin() -> front(), send_buffers_.begin() -> size()),
        [this](const boost::system::error_code &error, std::size_t length)
        {
            if (! error)
            {
                send_buffers_.pop_front();

                if (send_buffers_.size() > 0)
                {
                    transmit();
                }
            }
            else
            {
                send_buffers_.clear();

                hft2ctrader_log(ERROR) << "SSL Write data error ("
                                       << error.value() << "): "
                                       << error.message() << ".";

                if (connection_stage_ != connection_stage_type::CONNECTING)
                {
                    on_error(error);
                }
            }
        });
}

void ctrader_ssl_connection::try_reconnect_after_a_while(void)
{
    connection_attempts_++;

    if (connection_attempts_ == 3001)
    {
        hft2ctrader_log(ERROR) << "I can't connect to the cTrader server, tried over 3k times and give up";

        throw std::runtime_error("cTrader server connection error");
    }

    int delay;

    if (connection_attempts_ > 0 && connection_attempts_ < 10)
    {
        delay = 5;
    }
    else if (connection_attempts_ >= 10 && connection_attempts_ < 60)
    {
        delay = 25;
    }
    else
    {
        delay = 60;
    }

    reconnect_timer_.expires_after(boost::asio::chrono::seconds(delay));
    reconnect_timer_.async_wait(
        [this](const boost::system::error_code &error)
        {
            if (! error)
            {
                hft2ctrader_log(INFO) << "Attempt to connect to cTrader, trial #"
                                      << (connection_attempts_ + 1);

                connect();
            }
            else if (error == boost::asio::error::operation_aborted)
            {
                hft2ctrader_log(WARNING) << "Attempt to connect to cTrader aborted!";
            }
        }
    );
}

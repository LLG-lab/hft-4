/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System ® ≣≡=-              **
**                                                                    **
**          Copyright © 2017 - 2026 by LLG Ryszard Gradowski          **
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

#include <metrics.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <utility>
#include <sstream>

#include <easylogging++.h>

#include "../hft-config.h"

#undef HFT_DEVTEST

#ifdef HFT_DEVTEST
#include <iostream>
#endif

#define hft_log(__X__) \
    CLOG(__X__, "metrics")

namespace {

namespace beast = boost::beast;
namespace http  = beast::http;

using tcp = boost::asio::ip::tcp;

class
{
public:

    void setup_percentage_use_of_margin_metric(const std::string &market, const std::string &instrument, double value)
    {
        labels lbs{market, instrument};

        for (auto &item : percentage_use_of_margin_metrics_)
        {
            if (item.first == lbs)
            {
                item.second = value;

                return;
            }
        }

        percentage_use_of_margin_metrics_.emplace_back(lbs, value);
    }

    void setup_opened_positions_metric(const std::string &market, const std::string &instrument, int value)
    {
        labels lbs{market, instrument};

        for (auto &item : opened_positions_metrics_)
        {
            if (item.first == lbs)
            {
                item.second = value;

                return;
            }
        }

        opened_positions_metrics_.emplace_back(lbs, value);
    }

    std::string produce_metrics_text_format(void) const
    {
        std::ostringstream out;

        if (percentage_use_of_margin_metrics_.size())
        {
            out << "# HELP hft_percentage_use_of_margin How many in percentage money is used by instrument." << std::endl
                << "# TYPE hft_percentage_use_of_margin gauge" << std::endl;

            for (const auto &item : percentage_use_of_margin_metrics_)
            {
                out << "hft_percentage_use_of_margin{market=\"" << item.first.market_
                    << "\",instrument=\"" << item.first.instrument_ << "\"} "
                    << item.second << std::endl;
            }
        }

        if (opened_positions_metrics_.size())
        {
            out << "# HELP hft_opened_positions Total number of opened position by instrument." << std::endl
                << "# TYPE hft_opened_positions gauge" << std::endl;

            for (const auto &item : opened_positions_metrics_)
            {
                out << "hft_opened_positions{market=\"" << item.first.market_
                    << "\",instrument=\"" << item.first.instrument_ << "\"} "
                    << item.second << std::endl;
            }
        }

        return out.str();
    }

private:

    struct labels
    {
        labels(const std::string &market, const std::string &instrument)
            : market_{market}, instrument_{instrument} {}

        friend bool operator==(const labels &lhs, const labels &rhs)
        {
            return (lhs.market_ == rhs.market_) && (lhs.instrument_ == rhs.instrument_);
        }

        std::string market_;
        std::string instrument_;
    };

    std::vector<std::pair<labels, double>> percentage_use_of_margin_metrics_;
    std::vector<std::pair<labels, int>> opened_positions_metrics_;
} m;

bool metrics_service_enabled = false;

std::string hft_http_server_version(void)
{
    return std::string("HFT v.") + std::string(HFT_VERSION_MAJOR)
                                 + std::string(".")
                                 + std::string(HFT_VERSION_MINOR);
}

std::string fail_message(beast::error_code ec, char const* what)
{
    std::string res = what + std::string(": ") + ec.message();

    return res;
}

template <class Body, class Allocator>
http::message_generator handle_request(http::request<Body, http::basic_fields<Allocator>> &&req)
{
    //
    // Returns a bad request response.
    //

    auto const bad_request =
    [&req](beast::string_view why)
    {
        http::response<http::string_body> res{http::status::bad_request, req.version()};
        res.set(http::field::server, hft_http_server_version());
        res.set(http::field::content_type, "text/plain; version=0.0.4; charset=utf-8; escaping=values");
        res.keep_alive(req.keep_alive());
        res.body() = std::string(why);
        res.prepare_payload();

        return res;
    };

    //
    // Returns a not found response.
    //

    auto const not_found =
    [&req](beast::string_view target)
    {
        http::response<http::string_body> res{http::status::not_found, req.version()};
        res.set(http::field::server, hft_http_server_version());
        res.set(http::field::content_type, "text/plain; version=0.0.4; charset=utf-8; escaping=values");
        res.keep_alive(req.keep_alive());
        res.body() = "The resource '" + std::string(target) + "' was not found.";
        res.prepare_payload();

        return res;
    };

    //
    // Returns a server error response.
    //

    auto const server_error =
    [&req](beast::string_view what)
    {
        http::response<http::string_body> res{http::status::internal_server_error, req.version()};
        res.set(http::field::server, hft_http_server_version());
        res.set(http::field::content_type, "text/plain; version=0.0.4; charset=utf-8; escaping=values");
        res.keep_alive(req.keep_alive());
        res.body() = "An error occurred: '" + std::string(what) + "'";
        res.prepare_payload();

        return res;
    };

    //
    // Make sure we can handle the method.
    //

    if (req.method() != http::verb::get && req.method() != http::verb::head)
    {
        return bad_request("Unknown HTTP-method");
    }

    //
    // Only metrics target is supported.
    //

    if (req.target() != "/metrics")
    {
        return bad_request("Illegal request-target");
    }

    std::string body = m.produce_metrics_text_format();

    //
    // Respond to HEAD request.
    //

    if(req.method() == http::verb::head)
    {
        http::response<http::empty_body> res{http::status::ok, req.version()};
        res.set(http::field::server, hft_http_server_version());
        res.set(http::field::content_type, "text/plain; version=0.0.4; charset=utf-8; escaping=values");
        res.content_length(body.size());
        res.keep_alive(req.keep_alive());

        return res;
    }

    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, hft_http_server_version());
    res.set(http::field::content_type, "text/plain; version=0.0.4; charset=utf-8; escaping=values");
    res.keep_alive(req.keep_alive());
    res.body() = body;
    res.prepare_payload();

    return res;
}

//
// Handles an HTTP server connection.
//

class session : public std::enable_shared_from_this<session>
{
public:

    session(tcp::socket &&socket)
        : stream_(std::move(socket))
    {
    }

    #ifdef HFT_DEVTEST
    ~session(void)
    {
        std::cout << "session::~session() – Got called.\n";
    }
    #endif

    //
    // Start the asynchronous operation.
    //

    void run(void)
    {
        boost::asio::dispatch(stream_.get_executor(), beast::bind_front_handler(&session::do_read, shared_from_this()));
    }

    void do_read(void)
    {
        //
        // Make the request empty before reading,
        // otherwise the operation behavior is undefined.
        //

        req_ = {};

        //
        // Set the timeout.
        //

        stream_.expires_after(std::chrono::seconds(30));

        //
        // Read a request.
        //

        http::async_read(stream_, buffer_, req_, beast::bind_front_handler(&session::on_read, shared_from_this()));
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        //
        // This means they closed the connection.
        //

        if (ec == http::error::end_of_stream)
        {
            return do_close();
        }

        if (ec)
        {
            hft_log(ERROR) << fail_message(ec, "read");

            return;
        }

        //
        // Send the response.
        //

        send_response(handle_request(std::move(req_)));
    }

    void send_response(http::message_generator &&msg)
    {
        bool keep_alive = msg.keep_alive();

        //
        // Write the response.
        //

        beast::async_write(stream_, std::move(msg), beast::bind_front_handler(&session::on_write, shared_from_this(), keep_alive));
    }

    void on_write(bool keep_alive, beast::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec)
        {
            hft_log(ERROR) << fail_message(ec, "write");

            return;
        }

        if (! keep_alive)
        {
            //
            // This means we should close the connection, usually because
            // the response indicated the "Connection: close" semantic.
            //

            return do_close();
        }

        //
        // Read another request.
        //

        do_read();
    }

    void do_close()
    {
        //
        // Send a TCP shutdown.
        //

        beast::error_code ec;

        stream_.socket().shutdown(tcp::socket::shutdown_send, ec);

        //
        // At this point the connection is closed gracefully.
        //
    }

private:

    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> req_;
};

//
// Accepts incoming connections and launches the sessions.
//

class listener : public std::enable_shared_from_this<listener>
{
public:

    listener(boost::asio::io_context& ioc, tcp::endpoint endpoint)
        : ioc_(ioc), acceptor_(boost::asio::make_strand(ioc))
    {
        beast::error_code ec;

        //
        // Open the acceptor.
        //

        acceptor_.open(endpoint.protocol(), ec);

        if (ec)
        {
            auto msg = fail_message(ec, "open");

            hft_log(ERROR) << msg;

            throw std::runtime_error(msg);
        }

        //
        // Allow address reuse.
        //

        acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);

        if (ec)
        {
            auto msg = fail_message(ec, "set_option");

            hft_log(ERROR) << msg;

            throw std::runtime_error(msg);
        }

        //
        // Bind to the server address.
        //

        acceptor_.bind(endpoint, ec);

        if (ec)
        {
            auto msg = fail_message(ec, "bind");

            hft_log(ERROR) << msg;

            throw std::runtime_error(msg);
        }

        //
        // Start listening for connections.
        //

        acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);

        if (ec)
        {
            auto msg = fail_message(ec, "listen");

            hft_log(ERROR) << msg;

            throw std::runtime_error(msg);
        }
    }

    #ifdef HFT_DEVTEST
    ~listener(void)
    {
        std::cout << "listener::~listener() – Got called.\n";
    }
    #endif

    //
    // Start accepting incoming connections.
    //

    void run(void)
    {
        do_accept();
    }

private:

    void do_accept(void)
    {
        //
        // The new connection gets its own strand.
        //

        acceptor_.async_accept(boost::asio::make_strand(ioc_), beast::bind_front_handler(&listener::on_accept, shared_from_this()));
    }

    void on_accept(beast::error_code ec, tcp::socket socket)
    {
        if (ec)
        {
            auto msg = fail_message(ec, "accept");

            hft_log(ERROR) << msg;

            throw std::runtime_error(msg);
        }
        else
        {
            //
            // Create the session and run it.
            //

            #ifdef HFT_DEVTEST
            std::cout << "listener::on_accept(...) – Going to create & start HTTP session.\n";
            #endif

            std::make_shared<session>(std::move(socket)) -> run();
        }

        //
        // Accept another connection.
        //

        do_accept();

        #ifdef HFT_DEVTEST
        std::cout << "listener::on_accept(...) – Leaving method.\n";
        #endif
    }

    boost::asio::io_context& ioc_;
    tcp::acceptor acceptor_;
};

} // namespace

namespace metrics {

void create_server(boost::asio::io_context &ioctx, const std::string &addr, int p)
{
    el::Loggers::getLogger("metrics", true);

    auto const address = boost::asio::ip::make_address(addr);
    auto const port = static_cast<unsigned short>(p);

    #ifdef HFT_DEVTEST
    std::cout << "metrics::create_server() – Going to start HTTP listener.\n";
    #endif

    hft_log(INFO) << "Starting HTTP listener for metrics. Address: ‘"
                  << addr <<"’, port: ‘" << p << "’.";

    std::make_shared<listener>(ioctx, tcp::endpoint{address, port}) -> run();

    metrics_service_enabled = true;

    #ifdef HFT_DEVTEST
    std::cout << "metrics::create_server() – Leaving function.\n";
    #endif
}

bool is_service_enabled(void)
{
    return metrics_service_enabled;
}

void setup_percentage_use_of_margin(const std::string &market, const std::string &instrument, double value)
{
    if (metrics_service_enabled)
    {
        m.setup_percentage_use_of_margin_metric(market, instrument, value);
    }
}

void setup_opened_positions(const std::string &market, const std::string &instrument, int value)
{
    if (metrics_service_enabled)
    {
        m.setup_opened_positions_metric(market, instrument, value);
    }
}

} // namespace metrics

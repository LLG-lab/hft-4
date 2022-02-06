#include <boost/asio.hpp>
#include <ctime>
#include <chrono>
#include <iostream>

/*
    boost::asio::io_context ioctx_;
    boost::asio::steady_timer reconnect_timer_(ioctx_);

void foo()
{
    reconnect_timer_.expires_after(boost::asio::chrono::seconds(5));
    reconnect_timer_.async_wait(
        [=](const boost::system::error_code &error)
        {
            if (! error)
            {
                std::cout << "Attempt to connect to cTrader, trial #1\n";

                foo();
//                connect();
            }
            else if (error == boost::asio::error::operation_aborted)
            {
                std::cout << "Attempt to connect to cTrader aborted!\n";
            }
        }
    );

}
*/
int draft_main(int argc, char *argv[])
{
//    foo();
//    ioctx_.run();

    return 0;
}